#!/usr/bin/env python3
"""
Demo script for auto_palindromer that simulates LLM responses for testing
without requiring an actual API key.
"""

import os
import random
from auto_palindromer import AutoPalindromer

class MockAutoLPalindromer(AutoPalindromer):
    """Mock version that simulates LLM responses for testing"""
    
    def __init__(self):
        # Initialize with dummy API key since we won't use it
        super().__init__("mock_api_key")
        
    def query_llm(self, palindromes):
        """Mock LLM query that randomly selects a palindrome with some logic"""
        if not palindromes:
            return None
            
        print(f"Mock LLM: Evaluating {len(palindromes)} palindromes...")
        
        # Simple heuristics to pick "better" palindromes for demo:
        # 1. Prefer shorter palindromes (easier to understand)
        # 2. Prefer palindromes with spaces (more word-like)
        # 3. Add some randomness
        
        scored_palindromes = []
        for pal in palindromes:
            score = 0
            
            # Prefer shorter palindromes
            score += max(0, 50 - len(pal))
            
            # Prefer palindromes with reasonable number of spaces
            spaces = pal.count(' ')
            if 1 <= spaces <= 4:
                score += 20
                
            # Prefer palindromes without consecutive spaces
            if '  ' not in pal:
                score += 10
                
            # Add some randomness
            score += random.randint(0, 20)
            
            scored_palindromes.append((score, pal))
        
        # Sort by score and pick the best
        scored_palindromes.sort(reverse=True)
        selected = scored_palindromes[0][1]
        
        print(f"Mock LLM: Selected '{selected}' (score: {scored_palindromes[0][0]})")
        return selected

def run_demo():
    """Run a demonstration of the auto-palindromer"""
    print("Auto-Palindromer Demo")
    print("=" * 50)
    print("This demo uses a mock LLM that selects palindromes based on simple heuristics.")
    print("In real usage, replace this with actual Groq API calls.\n")
    
    if not os.path.exists("./Palindromer.exe"):
        print("❌ Palindromer.exe not found. Make sure the executable is present or build using Visual Studio.")
        return
    
    # Create mock auto-palindromer
    auto_pal = MockAutoLPalindromer()
    
    # Run demo with a few different starting points
    test_cases = [
        ("WAS|SAW", 3),
        ("STEP|PETS", 2),
    ]
    
    for starting_palindrome, iterations in test_cases:
        print(f"\nDemo: Starting with '{starting_palindrome}' for {iterations} iterations")
        print("-" * 60)
        
        try:
            history = auto_pal.iterate(starting_palindrome, iterations)
            print(f"\nDemo completed! Evolved through {len(history)} states:")
            for i, state in enumerate(history):
                print(f"  {i}: {state}")
        except Exception as e:
            print(f"Demo failed: {e}")
        
        print("\n" + "=" * 60)

if __name__ == "__main__":
    run_demo()