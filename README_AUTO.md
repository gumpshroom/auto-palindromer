# Auto-Palindromer

An automatic version of the palindrome builder that iterates upon itself using LLM evaluation to select the best palindromic phrases.

## How it works

1. **Generate Palindromes**: Runs the existing Palindromer program to generate a list of palindrome continuations
2. **LLM Evaluation**: Sends the generated palindromes to a Groq LLM API with the prompt: "using the following list of palindromic phrases, return only the single phrase that has the highest semantic value and makes the most logical sense"
3. **Selection**: Extracts the best palindrome from the LLM response
4. **Iteration**: Uses the selected palindrome as input for the next iteration
5. **Repeat**: Continues the process for a specified number of iterations

## Requirements

- Python 3.6+
- `requests` library (usually pre-installed)
- Compiled Palindromer executable
- Groq API key

## Setup

1. **Use the Palindromer.exe** (included) or **build using Visual Studio**:
   - Open `Palindromer.sln` in Visual Studio
   - Build the solution to generate `Palindromer.exe`

2. **Set up your API key**:
   ```cmd
   set GROQ_API_KEY=your_api_key_here
   ```

## Usage

```cmd
python auto_palindromer.py [starting_palindrome] [iterations] [api_key]
```

**Parameters:**
- `starting_palindrome`: Initial palindrome with `|` separator (e.g., "WAS|SAW")
- `iterations`: Number of iterations to run (default: 5)
- `api_key`: Groq API key (optional if GROQ_API_KEY env var is set)

**Examples:**

```bash
# Basic usage with environment variable
python3 auto_palindromer.py "WAS|SAW" 5

# With explicit API key
python3 auto_palindromer.py "NOTE NO|ONE TON" 3 $GROQ_API_KEY

# Default 5 iterations
python3 auto_palindromer.py "STEP|PETS"
```

## Sample Output

```
Starting automatic palindrome iteration with: WAS|SAW
Running for 3 iterations
============================================================

Iteration 1/3
Input: WAS|SAW
Palindromer output: Loading Dictionary dictionary.txt...
Loaded 49029 words.
Generating...
Found 412 continuations
Saving...
Done.
Generated 412 palindromes
Querying LLM for best palindrome...
LLM selected: WAS RAW | WARSAW

Iteration 2/3
Input: WAS RAW | WARSAW
...
```

## API Format

The script uses the Groq API with the following configuration:
- Model: `openai/gpt-oss-120b`
- Temperature: 1
- Max completion tokens: 8192
- Top-p: 1
- Reasoning effort: medium

## Features

- **Error Handling**: Graceful handling of API failures, timeouts, and invalid responses
- **Loop Detection**: Stops if the same palindrome is selected repeatedly
- **Progress Tracking**: Shows iteration history and selected palindromes
- **Format Conversion**: Automatically handles conversion between output and input formats
- **Flexible Input**: Supports various LLM response formats

## Testing

Run the test suite to verify functionality:

```bash
python3 test_auto_palindromer.py
```

This will test the integration with the Palindromer executable and response parsing without requiring an API key.

## Troubleshooting

**"Palindromer not found"**: Make sure to build the executable first
**"No API key provided"**: Set the GROQ_API_KEY environment variable or pass it as an argument
**"Error querying LLM"**: Check your API key and internet connection
**"Invalid palindrome format"**: Ensure your starting palindrome contains a `|` separator

## Files Created

The script creates intermediate files for each iteration:
- `palindromes_iter_1.txt`, `palindromes_iter_2.txt`, etc.

These files contain the palindromes generated at each iteration and can be useful for debugging or manual review.