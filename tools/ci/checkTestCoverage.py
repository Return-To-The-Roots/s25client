#!/usr/bin/env python3

import sys
import os


def parse_coverage(filepath):
    with open(filepath, mode='r') as f:
        file_to_lines = {}
        current_lines = []
        for line in f:
            if line.startswith('SF:'):
                filename = line[3:].rstrip('\n')
                current_lines = []
                file_to_lines[filename] = current_lines
            elif line.startswith('DA:'):
                line, hits = line[3:].rstrip('\n').split(',')[:2]
                current_lines.append((int(line), int(hits)))
    return file_to_lines


def is_source_line(line):
    line = line.strip()
    # Ignore empty lines, comments and single brackets
    return line and not line.startswith('//') and line not in '{}'


def classify_coverage(src_filepath, line_counts):
    with open(src_filepath, mode='r') as f:
        source_lines = f.read().split('\n')
    num_covered_lines = 0
    uncovered_lines = []
    for line, hits in line_counts:
        if line <= 0:
            raise IndexError('Invalid line: %s:%s' % (src_filepath, line))
        src_line = source_lines[line - 1]  # Line number to index
        if is_source_line(src_line):
            if hits:
                num_covered_lines += 1
            else:
                uncovered_lines.append('%s: %s' % (line, src_line))
    return num_covered_lines, uncovered_lines


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('Usage: %s <path-to-cov.info>' % sys.argv[0])
        sys.exit(1)
    coverage = parse_coverage(sys.argv[1])
    found_uncovered = False
    for src_filepath, line_counts in coverage.items():
        if not os.path.exists(src_filepath):
            print('WARNING: Ignoring non-existing file %s' % src_filepath)
            continue
        num_covered_lines, uncovered_lines = classify_coverage(src_filepath, line_counts)
        if uncovered_lines:
            covered_percent = num_covered_lines / (num_covered_lines + len(uncovered_lines)) * 100
            print('%s has only %.2f%% coverage. Uncovered lines:\n\t%s' %
                  (src_filepath, covered_percent, '\n\t'.join(uncovered_lines)))
            found_uncovered = True
    if found_uncovered:
        print('Found uncovered lines in tests. This is usually an error as all test code should be executed')
        print('In case those lines are unreachable by design(e.g. output operators or failure handling)')
        print('you can wrap those in LCOV_EXCL_START-LCOV_EXCL_STOP comments or use LCOV_EXCL_LINE')
        print('But this should happen only for well reasoned exceptions!')
        sys.exit(1)
