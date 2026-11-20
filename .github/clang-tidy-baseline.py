#!/usr/bin/env python3
"""Compare clang-tidy logs with the committed baseline.

The baseline counts existing diagnostics by file, check, and message.
Line numbers are ignored, so an edit that only shifts lines does not
reopen an existing finding. A finding is new when that count increases
or the file/check/message has not been seen before.

Record a fresh baseline from clang-tidy logs:

    python3 .github/clang-tidy-baseline.py record tidy-openssl.txt tidy-posix.txt > .github/clang-tidy-baseline.txt

Compare logs in CI:

    python3 .github/clang-tidy-baseline.py compare .github/clang-tidy-baseline.txt tidy-openssl.txt tidy-posix.txt
"""

import collections
import os
import re
import sys

DIAGNOSTIC = re.compile(
    r'^(?P<path>.+?):(?P<line>\d+):(?P<col>\d+): '
    r'(?P<severity>warning|error): (?P<message>.*?) \[(?P<check>[^\]]+)\]\s*$'
)
ROOTS = {
    'core',
    'cpp',
    'lib',
    'posix',
    'openssl',
    'windows',
    'freertos',
    'mbedtls',
    'qt',
}


def relative_path(path):
    path = path.replace('\\', '/')
    workspace = os.environ.get('GITHUB_WORKSPACE', '').replace('\\', '/')
    workspace = workspace.rstrip('/')
    if workspace and (path == workspace or path.startswith(workspace + '/')):
        return path[len(workspace) + 1:]
    parts = path.split('/')
    for index, part in enumerate(parts):
        if part in ROOTS:
            return '/'.join(parts[index:])
    return path


def tally(log_paths):
    """Return (counts, samples) keyed by (path, check, message)."""
    counts = collections.Counter()
    samples = {}
    for log_path in log_paths:
        with open(log_path, encoding='utf-8', errors='replace') as handle:
            for line in handle:
                match = DIAGNOSTIC.match(line.rstrip('\r\n'))
                if match is None:
                    continue
                key = (
                    relative_path(match.group('path')),
                    match.group('check'),
                    ' '.join(match.group('message').split()),
                )
                counts[key] += 1
                sample = '{}:{}:{}'.format(
                    key[0], match.group('line'), match.group('col'))
                samples.setdefault(key, [])
                if sample not in samples[key] and len(samples[key]) < 5:
                    samples[key].append(sample)
    return counts, samples


def load_baseline(baseline_path):
    counts = {}
    with open(baseline_path, encoding='utf-8') as handle:
        for line_number, raw in enumerate(handle, 1):
            line = raw.strip()
            if not line or line.startswith('#'):
                continue
            try:
                count_text, path, check, message = line.split('\t', 3)
                count = int(count_text)
            except ValueError:
                sys.exit('{}:{}: invalid baseline entry'.format(
                    baseline_path, line_number))
            counts[(path, check, message)] = count
    return counts


def write_baseline(counts, handle):
    handle.write(
        '# Existing clang-tidy findings. The workflow fails only when a\n'
        '# count below increases, or a new file/check/message appears.\n'
        '# count, path, check, and message are separated by tabs.\n'
    )
    for (path, check, message), count in sorted(counts.items()):
        handle.write('{}\t{}\t{}\t{}\n'.format(count, path, check, message))


def write_summary(current, baseline, new_items, path):
    current_total = sum(current.values())
    baseline_total = sum(baseline.values())
    new_total = sum(item[0] for item in new_items)
    lines = [
        '### clang-tidy',
        '',
        'Baselined findings: {}'.format(baseline_total),
        'Current findings: {}'.format(current_total),
        'New findings: {}'.format(new_total),
        '',
        'The job fails only for findings that are not already in '
        '`.github/clang-tidy-baseline.txt`.',
        '',
    ]
    if new_items:
        lines.append('New:')
        lines.append('')
        for delta, _count, _base, key, samples in new_items:
            path_text, check, message = key
            lines.append(
                '- `{}` `{}` (+{}): {}'.format(path_text, check, delta, message)
            )
            for sample in samples:
                lines.append('  - `{}`'.format(sample))
    with open(path, 'w', encoding='utf-8') as handle:
        handle.write('\n'.join(lines) + '\n')


def compare(baseline_path, log_paths):
    baseline = load_baseline(baseline_path)
    current, samples = tally(log_paths)
    new_items = []
    for key, count in sorted(current.items()):
        base = baseline.get(key, 0)
        if count > base:
            new_items.append((count - base, count, base, key, samples.get(key, [])))
    write_summary(current, baseline, new_items, 'clang-tidy-summary.md')
    if not new_items:
        print('No new clang-tidy findings.')
        return 0
    print('New clang-tidy findings: {}'.format(sum(item[0] for item in new_items)))
    for delta, count, base, key, locations in new_items:
        path, check, message = key
        print('{} (+{}, baseline {}, now {}) [{}] {}'.format(
            path, delta, base, count, check, message))
        for location in locations:
            print('  {}'.format(location))
    return 1


def main(argv):
    if len(argv) < 3 or argv[1] not in ('record', 'compare'):
        sys.exit('usage: clang-tidy-baseline.py record|compare BASELINE LOG [LOG...]')
    command = argv[1]
    if command == 'record':
        counts, _samples = tally(argv[2:])
        write_baseline(counts, sys.stdout)
        return 0
    if len(argv) < 4:
        sys.exit('usage: clang-tidy-baseline.py compare BASELINE LOG [LOG...]')
    return compare(argv[2], argv[3:])


if __name__ == '__main__':
    sys.exit(main(sys.argv))
