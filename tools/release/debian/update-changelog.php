#!/usr/bin/php

// Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

<?php

$project = "unknown";
$rev = "";

if ( $argc > 2 )
{
	$project = $argv[1];
	$from = $argv[2];
	$to = $argv[3];
	$rev = "$from..$to";
}

$git_log = shell_exec("git log $rev --date=short --format=%h%n%aN%n%aE%n%ai%n%s%n%b###");
$git_log_bits = explode('###' . PHP_EOL, $git_log);

$changelog = array();

foreach ( $git_log_bits as $log )
{
	$log = trim($log);
	$bits = explode(PHP_EOL, $log);

	if ( count($bits) > 1 )
	{
		$commit  = $bits[0];
		$author  = $bits[1];
		$email   = $bits[2];
		$date    = strtotime($bits[3]);
		$subject = $bits[4];
		$body    = "";
		if ( count($bits) > 5 )
			$body    = $bits[5];

		$changelog[] = array(
			'commit'  => $commit,
			'date'    => $date,
			'author'  => $author,
			'email'   => $email,
			'subject' => trim($subject),
			'body'    => trim($body),
		);
	}
}

reset($changelog);

foreach ( $changelog as $entry )
{
	$day  = date("Ymd", $entry['date']);

	$commit  = $entry['commit'];
	$author  = $entry['author'];
	$email   = $entry['email'];
	$date    = date("D, d M Y H:i:s O", $entry['date']);
	$subject = $entry['subject'];
	$body    = $entry['body'];

	fwrite(STDERR, "processing $commit\n");

	echo "$project ($day-$commit) precise; urgency=low".PHP_EOL;
	echo "".PHP_EOL;
	echo "  * $subject".PHP_EOL;
	echo "".PHP_EOL;
	if ( strlen($body) > 0 )
		echo "    ".str_replace(PHP_EOL, PHP_EOL."   ", $body).PHP_EOL.PHP_EOL;
	echo " -- $author <$email>  $date".PHP_EOL;
	echo "".PHP_EOL;
}
