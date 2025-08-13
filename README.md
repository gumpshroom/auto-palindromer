# Palindromer
This tool helps create palindromes by brute-forcing all continuations from any starting point.
For more information of how the software functions see this [YouTube Video](https://youtu.be/ap08_AGPh8s).

## Auto-Palindromer

**NEW**: An automatic version that iterates upon itself using LLM evaluation! The auto-palindromer generates palindromes, sends them to a Groq LLM API to select the best one, then uses that selection as input for the next iteration.

See [`README_AUTO.md`](README_AUTO.md) for detailed instructions on using the automatic version.

**Quick start:**
```bash
# Build the palindromer first
g++ -o Palindromer palindrome.cpp cmdLine.cpp trie.cpp -std=c++17

# Set your API key
export GROQ_API_KEY="your_api_key_here"

# Run automatic palindrome iteration
python3 auto_palindromer.py "WAS|SAW" 5
```

## How to use
Text is inputted by passing an uppercase string with `-t` that contains a `|` separator in the center.
This will generally look something like this: `t="WAS|SAW"`. Do not add punctuation.
The palindrome normally build from the outside to the center, but this can be reversed using `-r`.

There are two algorithms to generate continuations. The brute-force method (default) will find
all palindromes up to the next full words in both directions. This is usually best because you
won't miss anything that could come next and is equivalent to a breadth-first search.

The other option is a Monte-Carlo search. This uses randomness to explore branches deeply but
will generally not contain all possibilities. This is best when you're looking for some
inspiration to get started. To use Monte-Carlo search add `-m` to the command.

Palindromer by default uses `dictionary.txt` as the dictionary and `palindromes.txt` as the output file.
This can be changed with `-d="your_dictionary.txt"` and `-o="output_name.txt"`.

## An Example
Start with a list of all start and end palindrome pairs:

```./Palindromer```

After looking through `palindromes.txt`, suppose we would like to continue developing `NOTE NO| ONE TON`

```./Palindromer -t="NOTE NO| ONE TON"```

Next let's continue with: `NOTE NO ERASER |RES ARE ONE TON`.

```./Palindromer -t="NOTE NO ERASER |RES ARE ONE TON"```

Lastly, I find a way to finish the sentence with `NOTE: NO ERASER OF ORES ARE ONE TON.`
It may not be very meaningful, but the whole process took under 10 minutes.
