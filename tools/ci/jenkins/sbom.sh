#!/bin/bash

# Copyright (C) 2005 - 2025 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

###############################################################################
#
# This file will only be read by Jenkinsfile.
#
# Write a CycloneDX SBOM of the build environment (all installed packages of
# the toolchain image) to the result directory.
#
# Packages that end up in the product (the libraries it is built against and
# their runtime dependencies) get scope "required", everything else (compiler,
# SDKs, build tools) scope "excluded".
#
# Parameter marker (replaced by Jenkins):
# - %architecture% : i.e "windows.i686"
#
# Environment (set by Jenkins, optional):
# - RTTR_IMAGE_REF, RTTR_IMAGE_ID, RTTR_IMAGE_REPO_DIGEST : the toolchain image
#
# Directory structure:
# /result : directory for result files
#

set +x -euo pipefail

architecture="%architecture%"

sbom_dir=/result/sbom
mkdir -p "$sbom_dir"

echo "Architecture: $architecture"
syft --version

# Only the package database is relevant: toolchain, SDKs and libraries are all installed as Debian packages.
# No file components (hashes of every package-owned file): 40k+ entries without value.
export SYFT_FILE_METADATA_SELECTION=none

# Skip the (large) mounts and the Jenkins workspace
workspace=$(dirname "$PWD")
syft scan dir:/ --quiet \
    --select-catalogers dpkg \
    --exclude './.ccache/**' --exclude './source/**' --exclude './result/**' \
    --exclude "./${workspace#/}/**" \
    --exclude './proc/**' --exclude './sys/**' --exclude './dev/**' --exclude './tmp/**' --exclude './home/**' \
    --source-name "rttr-buildenv-$architecture" \
    --source-version "$(. /etc/os-release && echo "$VERSION_ID")" \
    -o "cyclonedx-json=$sbom_dir/buildenv-$architecture.cdx.json"

sbom_file="$sbom_dir/buildenv-$architecture.cdx.json"

# Packages that are linked into / shipped with the product. All their (transitive) runtime dependencies (not -dev packages) are runtime dependencies, too.
case "$architecture" in
    windows.i686)   runtime_roots='^mingw-w64-(boost|bzip2|curl|libiconv|libminiupnpc|libogg|libvorbis|sdl2|sdl2-mixer)-i686$' ;;
    windows.x86_64) runtime_roots='^mingw-w64-(boost|bzip2|curl|libiconv|libminiupnpc|libogg|libvorbis|sdl2|sdl2-mixer)-x86-64$' ;;
    apple.*)        runtime_roots='^apple-(uni-boost|framework-sdl2|framework-sdl2mixer|libminiupnpc)$' ;;
    linux.*)        runtime_roots='^(libboost-(filesystem|iostreams|locale|program-options)1\.92\.0|libsdl2-2\.0-0|libsdl2-mixer-2\.0-0|libcurl[0-9]+t64|libminiupnpc[0-9]+|libbz2-1\.0|liblzma[0-9]|libzstd[0-9])$' ;;
    *)              runtime_roots='^$' ;;
esac

# Describe the build environment as a container (not a file) and set the scope of all packages
jq --arg arch "$architecture" --arg roots "$runtime_roots" \
   --arg ref "${RTTR_IMAGE_REF:-}" --arg id "${RTTR_IMAGE_ID:-}" --arg digest "${RTTR_IMAGE_REPO_DIGEST:-}" '
  (.dependencies // [] | map({key: .ref, value: (.dependsOn // [])}) | from_entries) as $graph
  | [.components[] | select(.type == "library" and (.name | test($roots))) | ."bom-ref"] as $seed
  | ([.components[] | select(.name | test("-(dev|dev-bin|multidev)$")) | ."bom-ref"] | map({key: ., value: true}) | from_entries) as $dev
  | ({seen: ($seed | map({key: ., value: true}) | from_entries), todo: $seed}
     | until(.todo | length == 0;
         .seen as $seen
         | ([.todo[] | $graph[.] // [] | .[] | select(($seen[.] | not) and ($dev[.] | not))] | unique) as $new
         | {seen: ($seen + ($new | map({key: ., value: true}) | from_entries)), todo: $new})
     | .seen) as $required
  | .components |= map(if .type == "library"
                         then . + {scope: (if $required[."bom-ref"] then "required" else "excluded" end)}
                         else . end)
  | .metadata.component = ({
        type: "container",
        "bom-ref": ("buildenv:" + $arch),
        name: ("rttr-buildenv-" + $arch),
        version: (if $id != "" then $id else .metadata.component.version end),
        description: ("Toolchain image used to build RTTR for " + $arch),
        properties: ([{name: "rttr:architecture", value: $arch}]
                     + (if $ref != "" then [{name: "docker:image", value: $ref}] else [] end)
                     + (if $digest != "" then [{name: "docker:repo-digest", value: $digest}] else [] end))
      } + (if ($id | startswith("sha256:")) then {hashes: [{alg: "SHA-256", content: ($id | ltrimstr("sha256:"))}]} else {} end))
' "$sbom_file" > "$sbom_file.tmp"
mv "$sbom_file.tmp" "$sbom_file"

echo "Components: $(jq '.components | length' "$sbom_file")" \
     "(required: $(jq '[.components[] | select(.scope == "required")] | length' "$sbom_file"))"
