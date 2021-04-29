#!/usr/bin/php

// Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

<?php

$changelog = file_get_contents("changelog");

$pattern = "(:\n    )\n\n";
$replace = "\${1}Empty log message\n\n";
$changelog = preg_replace("/$pattern/", $replace, $changelog);

$pattern = "(  \* \[r([0-9]*)\])([^:]*):\n    ";
$replace = "  * ";
$changelog = preg_replace("/$pattern/s", $replace, $changelog);

$pattern = "(  \* \[r([0-9]*)\] \.:\n    )\n";
$replace = "\${1}Empty log message\n";
$changelog = preg_replace("/$pattern/", $replace, $changelog);

$pattern = "s25rttr \([0-9]{8}-[0-9]*\) unstable; urgency=low\n\n  \* (\[r([0-9]*)\] \.:\n    |)Empty log message\n\n -- ([^<>]+) <([^@]+)@siedler25\.org>  [A-Z][a-z]{2}, [0-9]{2} [A-Z][a-z]{2} [0-9]{4} [0-9]{2}:[0-9]{2}:[0-9]{2} \+[0-9]{4}\n\n";
$replace = "";
$changelog = preg_replace("/$pattern/", $replace, $changelog);

#print $changelog;

file_put_contents("changelog", $changelog);
