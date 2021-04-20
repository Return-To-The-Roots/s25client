#!/usr/bin/env python3

# Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

import argparse
from collections import namedtuple
import re
import os

IWYURecord = namedtuple('IWYURecord', ('toAdd', 'toRemove'))
_ADD_REMOVE_RES = {
    'toAdd': re.compile(r'^(.*) should add these lines:$'),
    'toRemove': re.compile(r'^(.*) should remove these lines:$')
}


def getLinesTillEmpty(f):
    result = []
    for line in f:
        if line.startswith('- '):
            line = line[2:]
        line = line.strip()
        if not line:
            break
        if not line.startswith('#include <boost/test'):
            result.append(line)
    return result


def parseOutput(filepath):
    result = {}
    with open(filepath) as f:
        for line in f:
            line = line.strip()
            for field, regexp in _ADD_REMOVE_RES.items():
                m = regexp.search(line)
                if m:
                    filename = m.group(1)
                    if filename not in result:
                        result[filename] = IWYURecord([], [])
                    getattr(result[filename],
                            field).extend(getLinesTillEmpty(f))
    return result


def getRecordsToModify(records):
    return {
        file: record
        for (file, record) in records.items() if record.toRemove
    }


def fixupFiles(records):
    for file, record in records.items():
        with open(file) as f, open(file + '.tmp', 'w') as fOut:
            for line in f:
                if not any(i in line for i in record.toRemove):
                    fOut.write(line)
        os.replace(file + '.tmp', file)


def main():
    parser = argparse.ArgumentParser(
        description="Remove includes as found by IWYU")
    parser.add_argument(
        "-i",
        "--input",
        type=str,
        required=True,
        help="The IWYU output.",
    )
    args = parser.parse_args()
    records = getRecordsToModify(parseOutput(args.input))
    fixupFiles(records)


if __name__ == "__main__":
    main()
