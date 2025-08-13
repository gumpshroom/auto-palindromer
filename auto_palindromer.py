#!/usr/bin/env python3
"""
Automatic Palindrome Iterator

This script creates an automatic version of the palindrome builder that iterates upon itself
after generating a list of palindromes using LLM evaluation.

Usage:
    python3 auto_palindromer.py [starting_palindrome] [iterations] [api_key]
    
Example:
    python3 auto_palindromer.py "WAS|SAW" 5 $GROQ_API_KEY
"""

import os
import sys
import json
import subprocess
import tempfile
import requests
import re
from typing import Optional, List, Set


def load_dictionary(dict_path: str = "dictionary.txt") -> Set[str]:
    """Load a dictionary file with one word per line (case-insensitive, allows spaces)."""
    if not os.path.exists(dict_path):
        print(f"Dictionary file {dict_path} not found!")
        return set()
    with open(dict_path, "r", encoding="utf-8") as f:
        # Keep original casing for spaces, but strip each line
        words = {line.strip() for line in f if line.strip()}
    return words


def filter_palindromes_with_dictionary(
    palindromes: List[str], dictionary: Set[str]
) -> List[str]:
    """
    For each palindrome of the form WORD|DROW (each side can contain spaces),
    split both sides by spaces, and only keep palindromes where all words
    on both sides are found in the dictionary (case-insensitive match).
    """
    filtered = []
    for line in palindromes:
        if "|" not in line:
            continue
        left, right = line.split("|", 1)
        left_words = [w.strip() for w in left.strip().split()]
        right_words = [w.strip() for w in right.strip().split()]
        # All words in both sides must be in the dictionary
        if all(word in dictionary for word in left_words + right_words):
            filtered.append(line)
    return filtered


class AutoPalindromer:
    def __init__(self, api_key: str, palindromer_path: str = "./Palindromer.exe", dict_path: str = "dictionary.txt"):
        self.api_key = api_key
        self.palindromer_path = palindromer_path
        self.api_url = "https://api.groq.com/openai/v1/chat/completions"
        # Load dictionary at initialization (case-sensitive, as dictionary.txt may contain phrases with spaces)
        self.dictionary = load_dictionary(dict_path)
        
    def run_palindromer(self, input_text: str, output_file: str = "palindromes.txt") -> bool:
        """Run the Palindromer program with given input"""
        try:
            cmd = [self.palindromer_path, f'-t={input_text}', f'-o={output_file}']
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
            
            if result.returncode != 0:
                print(f"Error running palindromer: {result.stderr}")
                return False
                
            print(f"Palindromer output: {result.stdout.strip()}")
            return True
            
        except subprocess.TimeoutExpired:
            print("Palindromer timed out")
            return False
        except Exception as e:
            print(f"Error running palindromer: {e}")
            return False
    
    def read_palindromes(self, file_path: str) -> List[str]:
        """Read palindromes from the output file"""
        try:
            with open(file_path, 'r') as f:
                palindromes = [line.strip() for line in f.readlines() if line.strip()]
            return palindromes
        except Exception as e:
            print(f"Error reading palindromes file: {e}")
            return []
    
    def filter_palindromes(self, palindromes: List[str]) -> List[str]:
        """Filter palindromes using the loaded dictionary."""
        if not self.dictionary:
            print("Warning: Dictionary is empty. No filtering applied.")
            return palindromes
        filtered = filter_palindromes_with_dictionary(palindromes, self.dictionary)
        print(f"Filtered {len(filtered)} palindromes from {len(palindromes)} using dictionary.")
        return filtered
    
    def query_llm(self, palindromes: List[str]) -> Optional[str]:
        """Query the LLM to select the best palindrome"""
        if not palindromes:
            return None
            
        # Create the prompt as specified in the problem statement
        palindromes_text = '\n'.join(palindromes)
        prompt = f"using the following list of palindromic phrases, return only the single phrase that has the highest semantic value and makes the most logical sense: {palindromes_text}"
        
        payload = {
            "messages": [
                {
                    "role": "user", 
                    "content": prompt
                }
            ],
            "model": "openai/gpt-oss-120b",
            "temperature": 1,
            "max_completion_tokens": 8192,
            "top_p": 1,
            "stream": False,  # Changed to False for easier parsing
            "reasoning_effort": "medium",
            "stop": None
        }
        
        headers = {
            "Content-Type": "application/json",
            "Authorization": f"Bearer {self.api_key}"
        }
        
        try:
            response = requests.post(self.api_url, json=payload, headers=headers, timeout=30)
            response.raise_for_status()
            
            data = response.json()
            content = data.get('choices', [{}])[0].get('message', {}).get('content', '').strip()
            
            if content:
                # Extract the palindrome from the response (remove any extra text)
                selected_palindrome = self.extract_palindrome(content, palindromes)
                return selected_palindrome
                
        except requests.exceptions.RequestException as e:
            print(f"Error querying LLM: {e}")
        except Exception as e:
            print(f"Error parsing LLM response: {e}")
            
        return None
    
    def extract_palindrome(self, llm_response: str, palindromes: List[str]) -> Optional[str]:
        """Extract the selected palindrome from LLM response"""
        # First, try to find an exact match
        for palindrome in palindromes:
            if palindrome in llm_response:
                return palindrome
        
        # If no exact match, look for the palindrome pattern in the response
        # Remove common response prefixes/suffixes
        cleaned_response = llm_response.strip()
        for prefix in ["The best palindrome is:", "I select:", "Selected:", "Answer:"]:
            if cleaned_response.startswith(prefix):
                cleaned_response = cleaned_response[len(prefix):].strip()
        
        # Look for palindrome-like patterns (containing |)
        lines = cleaned_response.split('\n')
        for line in lines:
            line = line.strip()
            if '|' in line:
                # This looks like a palindrome
                return line
                
        # If still nothing found, return the first palindrome as fallback
        return palindromes[0] if palindromes else None
    
    def convert_to_input_format(self, palindrome: str) -> str:
        """Convert a palindrome from output format back to input format"""
        # The palindromes.txt format appears to be: "START | END" or "START|END"
        # We need to return it in the same format for the next iteration
        if '|' in palindrome:
            return palindrome
        else:
            # If somehow there's no |, we can't process it
            raise ValueError(f"Invalid palindrome format: {palindrome}")
    
    def iterate(self, starting_palindrome: str, max_iterations: int = 5) -> List[str]:
        """Run the automatic palindrome iteration process"""
        current_input = starting_palindrome
        iteration_history = [current_input]
        
        print(f"Starting automatic palindrome iteration with: {current_input}")
        print(f"Running for {max_iterations} iterations")
        print("=" * 60)
        
        for i in range(max_iterations):
            print(f"\nIteration {i + 1}/{max_iterations}")
            print(f"Input: {current_input}")
            
            # Run palindromer
            output_file = f"palindromes_iter_{i+1}.txt"
            if not self.run_palindromer(current_input, output_file):
                print(f"Failed to run palindromer on iteration {i+1}")
                break
                
            # Read generated palindromes
            palindromes = self.read_palindromes(output_file)
            if not palindromes:
                print(f"No palindromes generated on iteration {i+1}")
                break

            # Filter palindromes by dictionary
            palindromes = self.filter_palindromes(palindromes)
            if not palindromes:
                print(f"No valid palindromes found after dictionary filtering on iteration {i+1}")
                break

            print(f"Generated {len(palindromes)} palindromes after filtering")
            
            # Query LLM for best palindrome
            print("Querying LLM for best palindrome...")
            selected = self.query_llm(palindromes)
            
            if not selected:
                print("Failed to get LLM selection")
                break
                
            print(f"LLM selected: {selected}")
            
            # Convert to next input format
            try:
                current_input = self.convert_to_input_format(selected)
                iteration_history.append(current_input)
            except ValueError as e:
                print(f"Failed to convert palindrome: {e}")
                break
                
            # Check if we're in a loop (same input as before)
            if current_input in iteration_history[:-1]:
                print("Detected loop, stopping iteration")
                break
                
        print("\n" + "=" * 60)
        print("Iteration complete!")
        print("History:")
        for i, palindrome in enumerate(iteration_history):
            print(f"  {i}: {palindrome}")
            
        return iteration_history


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 auto_palindromer.py [starting_palindrome] [iterations] [api_key]")
        print("Example: python3 auto_palindromer.py \"WAS|SAW\" 5")
        print("Note: GROQ_API_KEY environment variable must be set if api_key not provided")
        sys.exit(1)
    
    starting_palindrome = sys.argv[1]
    max_iterations = int(sys.argv[2]) if len(sys.argv) > 2 else 5
    api_key = sys.argv[3] if len(sys.argv) > 3 else os.environ.get('GROQ_API_KEY')
    
    if not api_key:
        print("Error: No API key provided. Set GROQ_API_KEY environment variable or pass as argument")
        sys.exit(1)
    
    auto_palindromer = AutoPalindromer(api_key)
    
    # Check if palindromer exists
    if not os.path.exists(auto_palindromer.palindromer_path):
        print(f"Error: Palindromer not found at {auto_palindromer.palindromer_path}")
        print("Make sure to build the palindromer first: g++ -o Palindromer palindrome.cpp cmdLine.cpp trie.cpp -std=c++17")
        sys.exit(1)
    
    try:
        auto_palindromer.iterate(starting_palindrome, max_iterations)
    except KeyboardInterrupt:
        print("\nInterrupted by user")
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()