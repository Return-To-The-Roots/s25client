#!/bin/bash

# Copyright (C) 2005 - 2025 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

###############################################################################
#
# This file will only be read by Jenkinsfile.
#
# Create a CycloneDX SBOM of the git submodules and merge it with the SBOMs of
# all build environments into a single result/sbom/rttr.cdx.json
#
# Licenses of the submodules: Either the explicit ones (REUSE: LICENSES/*.txt, or a Boost license file),
# otherwise the "global" ones of this repository (LICENSES/*.txt).
#
# Environment (set by Jenkins, optional): BUILD_URL, BUILD_NUMBER, JOB_NAME
#
# Directory structure:
# /source (read-only) : source directory
# /result             : directory for result files
#

set +x -euo pipefail

# .NET self-contained cyclonedx-cli: no ICU in the image
export DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1

src_dir=${SRC_DIR:-/source}
sbom_dir=${SBOM_DIR:-/result/sbom}
source_sbom=$sbom_dir/source.cdx.json
merged_sbom=$sbom_dir/rttr.cdx.json

git_src() { git -c safe.directory='*' -C "$src_dir" "$@"; }

# SPDX expression from a REUSE LICENSES directory: "Apache-2.0 AND CC0-1.0 ..."
reuse_licenses() {
    local dir=$1/LICENSES
    [ -d "$dir" ] || return 0
    (cd "$dir" && ls -- *.txt 2>/dev/null | sed 's/\.txt$//' | sort | paste -sd'~' - | sed 's/~/ AND /g')
}

# Prints "<SPDX expression>\t<source of the information>"
submodule_license() {
    local dir=$1 expr
    expr=$(reuse_licenses "$dir")
    if [ -n "$expr" ] ; then
        printf '%s\tLICENSES\n' "$expr"
    elif grep -qsi 'Boost Software License' "$dir"/LICENSE* ; then
        printf 'BSL-1.0\tLICENSE\n'
    else
        printf '%s\tglobal\n' "$global_licenses"
    fi
}

global_licenses=$(reuse_licenses "$src_dir")
version=$(git_src describe --tags --always)
commit=$(git_src rev-parse HEAD)
root_ref="rttr@$version"

# One line per (recursive) submodule: path, commit, url, description, license, license source
submodules=$(git_src submodule foreach --quiet --recursive \
    'printf "%s\t%s\t%s\t%s\n" "$displaypath" "$sha1" "$(git config --get remote.origin.url)" "$(git describe --tags --always)"' \
    | while IFS=$'\t' read -r path rev url desc ; do
        printf '%s\t%s\t%s\t%s\t%s\n' "$path" "$rev" "$url" "$desc" "$(submodule_license "$src_dir/$path")"
    done)

printf '%s\n' "$submodules" | jq -R -s \
    --arg version "$version" --arg commit "$commit" '
  def purl($url; $rev):
    ($url | capture("github\\.com[:/](?<o>[^/]+)/(?<r>[^/]+?)(\\.git)?$")?)
    | if . then "pkg:github/\(.o)/\(.r)@\($rev)" else null end;
  {
    bomFormat: "CycloneDX",
    specVersion: "1.6",
    version: 1,
    metadata: {
      component: { type: "application", "bom-ref": "rttr-source", name: "rttr-source", version: $version,
                   properties: [ { name: "git:commit", value: $commit } ] }
    },
    components: [
      split("\n") | map(select(length > 0) | split("\t"))[]
      | { path: .[0], rev: .[1], url: (.[2] | sub("\\.git$"; "")), desc: .[3], license: .[4], licenseSource: .[5] }
      | { type: "library",
          "bom-ref": ("submodule:" + .path),
          name: (.path | split("/") | last),
          version: .rev,
          purl: purl(.url; .rev),
          licenses: (if .license != "" then [ { expression: .license } ] else null end),
          externalReferences: [ { type: "vcs", url: .url } ],
          properties: [ { name: "git:path", value: .path }, { name: "git:describe", value: .desc },
                        { name: "license:source", value: .licenseSource } ] }
      | with_entries(select(.value != null))
    ]
  }' > "$source_sbom"

echo "Submodules: $(jq '.components | length' "$source_sbom")"

cyclonedx merge --hierarchical --name rttr --version "$version" \
    --input-files "$source_sbom" "$sbom_dir"/buildenv-*.cdx.json \
    --output-format json --output-file "$merged_sbom"

# Enrich the root component and the metadata, add the dependencies between the parts, drop empty sections
jq --arg root_ref "$root_ref" --arg version "$version" --arg commit "$commit" --arg licenses "$global_licenses" \
   --arg cdx_version "$(cyclonedx --version | cut -d+ -f1)" \
   --arg build_url "${BUILD_URL:-}" --arg build_number "${BUILD_NUMBER:-}" --arg job_name "${JOB_NAME:-}" '
  (.components | map(."bom-ref")) as $parts
  | (.components | map(select(.name == "rttr-source")) | .[0].components // [] | map(."bom-ref")) as $submodules
  | (.components | map(select(.name == "rttr-source")) | .[0]."bom-ref") as $source_ref
  | .metadata.component += {
        type: "application",
        "bom-ref": $root_ref,
        description: "Return To The Roots - open source remake of The Settlers II",
        supplier: { name: "Settlers Freaks", url: ["https://www.rttr.info"], contact: [ { email: "sf-team@siedler25.org" } ] },
        licenses: (if $licenses != "" then [ { expression: $licenses } ] else [] end),
        purl: "pkg:github/Return-To-The-Roots/s25client@\($commit)",
        externalReferences: ([
            { type: "website", url: "https://www.rttr.info" },
            { type: "vcs", url: "https://github.com/Return-To-The-Roots/s25client" },
            { type: "issue-tracker", url: "https://github.com/Return-To-The-Roots/s25client/issues" }
          ] + (if $build_url != "" then [ { type: "build-system", url: $build_url } ] else [] end)),
        properties: [ { name: "git:commit", value: $commit } ]
      }
  | .metadata.lifecycles = [ { phase: "build" } ]
  | .metadata.tools.components = ((.metadata.tools.components // [])
        + [ { type: "application", author: "CycloneDX", name: "cyclonedx-cli", version: $cdx_version } ])
  | .metadata.properties = ((.metadata.properties // [])
        + (if $job_name != "" then [ { name: "jenkins:job", value: $job_name } ] else [] end)
        + (if $build_number != "" then [ { name: "jenkins:build", value: $build_number } ] else [] end))
  | .dependencies = ((.dependencies // []) + [
        { ref: $root_ref, dependsOn: $parts },
        { ref: $source_ref, dependsOn: $submodules }
      ] | group_by(.ref) | map({ref: .[0].ref, dependsOn: (map(.dependsOn // []) | add | unique)}))
  | del(.declarations, .definitions)
  | with_entries(select(.value != [] and .value != {}))
' "$merged_sbom" > "$merged_sbom.tmp"
mv "$merged_sbom.tmp" "$merged_sbom"

echo "Merged SBOM: $(jq '[.components[]] | length' "$merged_sbom") top-level components -> $merged_sbom"
