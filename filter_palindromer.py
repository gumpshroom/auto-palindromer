#!/usr/bin/env python3
"""
Run Palindromer.exe once and filter palindromes using dictionary.txt

Usage:
    python3 filter_palindromer_output.py "WAS|SAW" [-o outputfile] [-d dictionary.txt] [-p palindromer.exe]

- Calls Palindromer.exe with the given input and output file.
- Loads dictionary.txt (one word or phrase per line).
- Reads the output file, filters only palindromes where ALL words on both sides of the "|" are found in the dictionary.
- Prints valid palindromes to stdout (or optionally to a file).
"""

import argparse
import subprocess
import os
import sys

def load_dictionary(dict_path):
    if not os.path.exists(dict_path):
        print(f"Dictionary file {dict_path} not found!", file=sys.stderr)
        sys.exit(1)
    with open(dict_path, "r", encoding="utf-8") as f:
        return {line.strip() for line in f if line.strip()}

def filter_palindromes(palindromes, dictionary):
    filtered = []
    for line in palindromes:
        if "|" not in line:
            continue
        left, right = line.split("|", 1)
        left_words = [w.strip() for w in left.strip().split()]
        right_words = [w.strip() for w in right.strip().split()]
        if all(word in dictionary for word in left_words + right_words):
            filtered.append(line)
    return filtered

def main():
    parser = argparse.ArgumentParser(description="Run Palindromer.exe and filter output based on dictionary.txt")
    parser.add_argument("input", help="Input palindrome (e.g., 'WAS|SAW')")
    parser.add_argument("-o", "--output", default="palindromes_output.txt", help="Temporary output file from Palindromer.exe")
    parser.add_argument("-d", "--dictionary", default="dictionary.txt", help="Dictionary file")
    parser.add_argument("-p", "--palindromer", default="./Palindromer.exe", help="Path to Palindromer.exe")
    parser.add_argument("-f", "--filtered", help="If set, write filtered results to this file instead of stdout")
    args = parser.parse_args()

    # Run Palindromer.exe
    if not os.path.isfile(args.palindromer):
        print(f"Palindromer not found at {args.palindromer}", file=sys.stderr)
        sys.exit(1)

    cmd = [args.palindromer, f'-t={args.input}', f'-o={args.output}']
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
        if result.returncode != 0:
            print(f"Palindromer error: {result.stderr}", file=sys.stderr)
            sys.exit(result.returncode)
    except subprocess.TimeoutExpired:
        print("Palindromer.exe timed out", file=sys.stderr)
        sys.exit(1)

    # Load dictionary
    dictionary = load_dictionary(args.dictionary)

    # Read and filter palindromes
    try:
        with open(args.output, "r", encoding="utf-8") as f:
            palindromes = [line.strip() for line in f if line.strip()]
    except Exception as e:
        print(f"Error reading palindromes file: {e}", file=sys.stderr)
        sys.exit(1)

    filtered = filter_palindromes(palindromes, dictionary)

    # Output results
    output_lines = "\n".join(filtered)
    if args.filtered:
        with open(args.filtered, "w", encoding="utf-8") as f:
            f.write(output_lines + "\n" if output_lines else "")
    else:
        print(output_lines)

if __name__ == "__main__":
    main()