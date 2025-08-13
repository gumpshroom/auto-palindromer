#!/bin/bash

# Auto-Palindromer Shell Script
# This script implements the automatic palindrome iteration using curl
# exactly as specified in the problem statement.

set -e

# Configuration
PALINDROMER="./Palindromer"
API_URL="https://api.groq.com/openai/v1/chat/completions"
MAX_ITERATIONS=5
DEFAULT_INPUT="WAS|SAW"

# Check if required tools are available
check_requirements() {
    if [ ! -f "$PALINDROMER" ]; then
        echo "Error: Palindromer executable not found at $PALINDROMER"
        echo "Build it first: g++ -o Palindromer palindrome.cpp cmdLine.cpp trie.cpp -std=c++17"
        exit 1
    fi
    
    if ! command -v curl &> /dev/null; then
        echo "Error: curl not found"
        exit 1
    fi
    
    if ! command -v jq &> /dev/null; then
        echo "Error: jq not found (needed for JSON parsing)"
        echo "Install with: apt-get install jq"
        exit 1
    fi
    
    if [ -z "$GROQ_API_KEY" ]; then
        echo "Error: GROQ_API_KEY environment variable not set"
        exit 1
    fi
}

# Run the palindromer with given input
run_palindromer() {
    local input="$1"
    local output_file="$2"
    
    echo "Running Palindromer with input: $input"
    $PALINDROMER -t="$input" -o="$output_file"
}

# Query the LLM using curl exactly as specified in the problem statement
query_llm() {
    local palindromes_file="$1"
    
    if [ ! -f "$palindromes_file" ]; then
        echo "Error: Palindromes file $palindromes_file not found"
        return 1
    fi
    
    # Read palindromes from file
    local palindromes_content
    palindromes_content=$(cat "$palindromes_file")
    
    # Create the prompt as specified
    local prompt="using the following list of palindromic phrases, return only the single phrase that has the highest semantic value and makes the most logical sense: $palindromes_content"
    
    echo "Querying LLM..."
    
    # Use curl exactly as specified in the problem statement
    local response
    response=$(curl -s "$API_URL" \
        -X POST \
        -H "Content-Type: application/json" \
        -H "Authorization: Bearer ${GROQ_API_KEY}" \
        -d "{
             \"messages\": [
               {
                 \"role\": \"user\",
                 \"content\": \"$prompt\"
               }
             ],
             \"model\": \"openai/gpt-oss-120b\",
             \"temperature\": 1,
             \"max_completion_tokens\": 8192,
             \"top_p\": 1,
             \"stream\": false,
             \"reasoning_effort\": \"medium\",
             \"stop\": null
           }")
    
    if [ $? -ne 0 ]; then
        echo "Error: curl request failed"
        return 1
    fi
    
    # Extract the content from the response
    local selected_palindrome
    selected_palindrome=$(echo "$response" | jq -r '.choices[0].message.content' 2>/dev/null)
    
    if [ "$selected_palindrome" = "null" ] || [ -z "$selected_palindrome" ]; then
        echo "Error: Failed to parse LLM response"
        echo "Response: $response"
        return 1
    fi
    
    echo "LLM response: $selected_palindrome"
    
    # Extract palindrome from response (look for lines containing |)
    local extracted_palindrome
    extracted_palindrome=$(echo "$selected_palindrome" | grep -o '[^|]*|[^|]*' | head -1)
    
    if [ -z "$extracted_palindrome" ]; then
        # If no | found, try to find a palindrome in the palindromes file that matches
        while IFS= read -r line; do
            if echo "$selected_palindrome" | grep -q "$line"; then
                extracted_palindrome="$line"
                break
            fi
        done < "$palindromes_file"
    fi
    
    if [ -z "$extracted_palindrome" ]; then
        echo "Error: Could not extract palindrome from LLM response"
        return 1
    fi
    
    echo "$extracted_palindrome"
}

# Main iteration function
iterate_palindromes() {
    local current_input="$1"
    local iterations="$2"
    
    echo "Starting automatic palindrome iteration"
    echo "Initial input: $current_input"
    echo "Iterations: $iterations"
    echo "================================================"
    
    local history=("$current_input")
    
    for ((i=1; i<=iterations; i++)); do
        echo ""
        echo "Iteration $i/$iterations"
        echo "Input: $current_input"
        
        # Generate palindromes
        local output_file="palindromes_iter_$i.txt"
        if ! run_palindromer "$current_input" "$output_file"; then
            echo "Error: Palindromer failed on iteration $i"
            break
        fi
        
        local num_palindromes
        num_palindromes=$(wc -l < "$output_file")
        echo "Generated $num_palindromes palindromes"
        
        # Query LLM
        local selected_palindrome
        if ! selected_palindrome=$(query_llm "$output_file"); then
            echo "Error: LLM query failed on iteration $i"
            break
        fi
        
        echo "Selected: $selected_palindrome"
        
        # Check for loops
        for prev in "${history[@]}"; do
            if [ "$selected_palindrome" = "$prev" ]; then
                echo "Detected loop, stopping iteration"
                break 2
            fi
        done
        
        # Prepare for next iteration
        current_input="$selected_palindrome"
        history+=("$current_input")
    done
    
    echo ""
    echo "================================================"
    echo "Iteration complete!"
    echo "History:"
    for i in "${!history[@]}"; do
        echo "  $i: ${history[$i]}"
    done
}

# Main function
main() {
    local input="${1:-$DEFAULT_INPUT}"
    local iterations="${2:-$MAX_ITERATIONS}"
    
    echo "Auto-Palindromer Shell Script"
    echo "============================="
    
    check_requirements
    iterate_palindromes "$input" "$iterations"
}

# Show usage if no arguments
if [ $# -eq 0 ]; then
    echo "Usage: $0 [starting_palindrome] [iterations]"
    echo "Example: $0 \"WAS|SAW\" 5"
    echo ""
    echo "Requirements:"
    echo "- GROQ_API_KEY environment variable must be set"
    echo "- Palindromer executable must exist"
    echo "- curl and jq must be installed"
    exit 1
fi

main "$@"