#!/usr/bin/env python3
"""
Test script for auto_palindromer without requiring actual API calls
"""

import os
import sys
from auto_palindromer import AutoPalindromer

def test_palindromer_integration():
    """Test that we can run the Palindromer and read its output"""
    # Create a test instance (dummy API key)
    auto_pal = AutoPalindromer("dummy_key")
    
    print("Testing Palindromer integration...")
    
    # Test running palindromer
    test_input = "WAS|SAW"
    test_output = "test_palindromes.txt"
    
    print(f"Running palindromer with input: {test_input}")
    success = auto_pal.run_palindromer(test_input, test_output)
    
    if not success:
        print("❌ Failed to run palindromer")
        return False
        
    print("✅ Palindromer ran successfully")
    
    # Test reading output
    palindromes = auto_pal.read_palindromes(test_output)
    
    if not palindromes:
        print("❌ Failed to read palindromes")
        return False
        
    print(f"✅ Read {len(palindromes)} palindromes")
    print("First few palindromes:")
    for i, pal in enumerate(palindromes[:5]):
        print(f"  {i+1}: {pal}")
    
    # Test format conversion
    test_palindrome = palindromes[0]
    try:
        converted = auto_pal.convert_to_input_format(test_palindrome)
        print(f"✅ Format conversion: {test_palindrome} -> {converted}")
    except Exception as e:
        print(f"❌ Format conversion failed: {e}")
        return False
    
    # Clean up
    if os.path.exists(test_output):
        os.remove(test_output)
    
    return True

def test_extract_palindrome():
    """Test palindrome extraction from LLM response"""
    auto_pal = AutoPalindromer("dummy_key")
    
    test_palindromes = ["WAS A |ASAW", "WAS RAW | WARSAW", "WAS | SAW"]
    
    # Test various response formats
    test_cases = [
        ("WAS RAW | WARSAW", "WAS RAW | WARSAW"),
        ("The best palindrome is: WAS A |ASAW", "WAS A |ASAW"),
        ("I select: WAS | SAW\nThis makes the most sense.", "WAS | SAW"),
        ("WAS RAW | WARSAW is my choice", "WAS RAW | WARSAW"),
    ]
    
    print("\nTesting palindrome extraction...")
    
    for response, expected in test_cases:
        result = auto_pal.extract_palindrome(response, test_palindromes)
        if result == expected:
            print(f"✅ '{response}' -> '{result}'")
        else:
            print(f"❌ '{response}' -> '{result}' (expected '{expected}')")
    
    return True

if __name__ == "__main__":
    print("Running Auto-Palindromer Tests")
    print("=" * 40)
    
    if not os.path.exists("./Palindromer"):
        print("❌ Palindromer executable not found. Build it first:")
        print("   g++ -o Palindromer palindrome.cpp cmdLine.cpp trie.cpp -std=c++17")
        sys.exit(1)
    
    test_palindromer_integration()
    test_extract_palindrome()
    
    print("\n✅ All tests completed!")