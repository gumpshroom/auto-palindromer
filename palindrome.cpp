#include "cmdLine.h"
#include "trie.h"
#include <cassert>
#include <ctime>
#include <fstream>
#include <iostream>
#include <array>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <random>
#include <algorithm>

//=====  Brute-force parameters  =====
#define MAX_PALINDROMES 2000000
#define MAX_DEPTH 30

//=====  Monte-Carlo search parameters =====
//Try to stop after this length
#define STOP_LENGTH 100
//Max possible length
#define MAX_LENGTH 120
//Max number to generate
#define MAX_ITERS_GEN 10000000
//Probability of continuing if this is a word
#define PROB_CONT 0.75f

//=====  Filtering parameters  =====
//Maximum tokens for API (8000 TPM limit - be conservative)
#define MAX_TOKENS 1000
//Minimum word length to avoid nonsensical short words
#define MIN_WORD_LENGTH 2

//Using global variables makes the recursive calls more compact
Trie g_tri_for;
Trie g_tri_bac;

//Random number generator
std::mt19937 mt(std::time(0));

//Dictionary should be list of words separated by newlines
bool LoadDictionary(const std::string& fname) {
  int num_words = 0;
  std::ifstream fin(fname);
  if (!fin.is_open()) { return false; }
  std::string line;
  std::unordered_set<std::string> all_words;
  while (std::getline(fin, line)) {
    for (auto& c : line) c = toupper(c);
    g_tri_for.add(line);
    std::reverse(line.begin(), line.end());
    g_tri_bac.add(line);
    num_words += 1;
  }
  std::cout << "Loaded " << num_words << " words." << std::endl;
  return true;
}

bool CanAddAnyLetter(int startIx, Trie* tri_for, Trie* tri_bac, int& ix, bool endable) {
  Trie::CircIter circIter = tri_for->circIter(startIx);
  while (circIter.next()) {
    ix = circIter.getIx();
    if (tri_bac == nullptr || tri_bac->hasIx(ix) || (endable && tri_bac->isWord)) {
      return true;
    }
  }
  return false;
}

bool FindFirstWordEnd(Trie* trie, char& c) {
  if (trie == nullptr) { return false; }
  for (int i = 0; i < NUM_LETTERS; ++i) {
    Trie* branch = trie->nodes[i];
    if (branch != nullptr && branch->isWord) {
      c = (char)('A' + i);
      return true;
    }
  }
  return false;
}

void BruteSearchRec(Trie* triFor, Trie* triBac, std::vector<char>& strFor, std::vector<char>& strBac, int depth, std::set<std::string>& palindromes) {
  if (palindromes.size() > MAX_PALINDROMES) { return; }
  if (depth >= MAX_DEPTH || depth < 0) {
    std::string str_for(strFor.begin(), strFor.end());
    std::string str_bac(strBac.begin(), strBac.end());
    std::reverse(str_bac.begin(), str_bac.end());
    palindromes.insert(str_for + '|' + str_bac);
    return;
  }

  if (triFor->isWord) {
    //Try iteration with a space on the forward side
    strFor.push_back(' ');
    const int numSpacesFor = std::count(strFor.begin(), strFor.end(), ' ');
    const int numSpacesBac = std::count(strBac.begin(), strBac.end(), ' ');
    const bool forceEnd = (numSpacesBac >= 2 || numSpacesFor >= 2 || (!strBac.empty() && strBac.back() == ' '));
    BruteSearchRec(&g_tri_for, triBac, strFor, strBac, (forceEnd ? -1 : depth + 1), palindromes);
    strFor.pop_back();
  }
  if (triBac->isWord) {
    //Try iteration with a space on the backward side
    strBac.push_back(' ');
    const int numSpacesFor = std::count(strFor.begin(), strFor.end(), ' ');
    const int numSpacesBac = std::count(strBac.begin(), strBac.end(), ' ');
    const bool forceEnd = (numSpacesBac >= 2 || numSpacesFor >= 2 || (!strFor.empty() && strFor.back() == ' '));
    BruteSearchRec(triFor, &g_tri_bac, strFor, strBac, (forceEnd ? -1 : depth + 1), palindromes);
    strBac.pop_back();
  }

  //Try iterations with no spaces
  Trie::PalIter iter = triFor->palIter(triBac);
  while (iter.next()) {
    char c = iter.getLetter();
    strFor.push_back(c);
    strBac.push_back(c);
    BruteSearchRec(iter.getFor(), iter.getBac(), strFor, strBac, depth + 1, palindromes);
    strFor.pop_back();
    strBac.pop_back();
  }
}

void BruteSearch(std::set<std::string>& palindromes, const std::string& startFor, const std::string& startBac) {
  std::vector<char> strFor;
  std::vector<char> strBac;
  Trie* triFor = &g_tri_for;
  Trie* triBac = &g_tri_bac;
  const size_t forIxSize = startFor.find_last_of(' ');
  const size_t bacIxSize = startBac.find_first_of(' ');
  int forIx = (forIxSize == std::string::npos ? 0 : ((int)forIxSize) + 1);
  int bacIx = (bacIxSize == std::string::npos ? startBac.length() - 1 : ((int)bacIxSize) - 1);
  while (forIx < (int)startFor.length()) {
    const int ix = startFor[forIx] - 'A';
    if (ix < 0 || ix >= NUM_LETTERS) { return; }
    triFor = triFor->nodes[ix];
    if (triFor == nullptr) { return; }
    forIx += 1;
  }
  while (bacIx >= 0) {
    const int ix = startBac[bacIx] - 'A';
    if (ix < 0 || ix >= NUM_LETTERS) { return; }
    triBac = triBac->nodes[ix];
    if (triBac == nullptr) { return; }
    bacIx -= 1;
  }
  BruteSearchRec(triFor, triBac, strFor, strBac, 0, palindromes);
}

void RandSearch(std::set<std::string>& palindromes, const std::string& startFor, const std::string& startBac) {
  std::vector<char> v_for;
  std::vector<char> v_bac;
  Trie* triFor = &g_tri_for;
  Trie* triBac = &g_tri_bac;
  const size_t forIxSize = startFor.find_last_of(' ');
  const size_t bacIxSize = startBac.find_first_of(' ');
  int forIx = (forIxSize == std::string::npos ? 0 : ((int)forIxSize) + 1);
  int bacIx = (bacIxSize == std::string::npos ? startBac.length() - 1 : ((int)bacIxSize) - 1);
  while (forIx < (int)startFor.length()) {
    const int ix = startFor[forIx] - 'A';
    if (ix < 0 || ix >= NUM_LETTERS) { return; }
    triFor = triFor->nodes[ix];
    if (triFor == nullptr) { return; }
    forIx += 1;
  }
  while (bacIx >= 0) {
    const int ix = startBac[bacIx] - 'A';
    if (ix < 0 || ix >= NUM_LETTERS) { return; }
    triBac = triBac->nodes[ix];
    if (triBac == nullptr) { return; }
    bacIx -= 1;
  }

  //Initialize distributions
  std::uniform_int_distribution<int> randIx(0, NUM_LETTERS - 1);
  std::uniform_real_distribution<float> rand(0.0f, 1.0f);
  Trie* tri_for;
  Trie* tri_bac;

  for (int i = 0; i < MAX_ITERS_GEN; ++i) {
    tri_for = triFor;
    tri_bac = triBac;
    v_for.clear();
    v_bac.clear();

    while (v_for.size() < MAX_LENGTH) {
      //If this got to a leaf node, we must start a new word.
      //Or if this could be the end of the word, start a new word with some probability
      if (tri_for->isLeaf() || (tri_for->isWord && rand(mt) > PROB_CONT)) {
        v_for.push_back(' ');
        tri_for = &g_tri_for;
      }

      //Choose random start for search
      int startIx = randIx(mt);

      //Check if any forward letters are valid backward letters
      int ix = -1;
      bool canAddLetter = CanAddAnyLetter(startIx, tri_for, tri_bac, ix, true);

      //If nothing could be valid, end now
      if (!canAddLetter && !tri_bac->isWord) { break; }

      //If we must end the word or we can with some probability, do it
      assert(canAddLetter);
      if (tri_bac->isWord && (!tri_bac->hasIx(ix) || rand(mt) > PROB_CONT)) {
        //It was already a valid word, end it first
        v_bac.push_back(' ');
        tri_bac = &g_tri_bac;
        //Find the new index and make sure the letter can be added
        canAddLetter = CanAddAnyLetter(startIx, tri_for, tri_bac, ix, true);
        if (!canAddLetter) { break; }
        assert(tri_bac->hasIx(ix));
      }

      //At this point the letter should be able to add for both directions
      const char c = (char)('A' + ix);
      assert(tri_for->hasIx(ix));
      assert(tri_bac->hasIx(ix));
      v_for.push_back(c);
      v_bac.push_back(c);
      tri_for = tri_for->decend(ix);
      tri_bac = tri_bac->decend(ix);

      //If past the stop length, quit as soon a valid ending is found.
      if (v_for.size() >= STOP_LENGTH && tri_for->isWord && tri_bac->isWord) {
        break;
      }
    }

    //Sentence is done, see if it can end
    bool isValid = false;
    if (tri_for->isWord && tri_bac->isWord) {
      isValid = true;
    } else if (tri_for->isWord) {
      char endChar;
      if (FindFirstWordEnd(tri_bac, endChar)) {
        v_bac.push_back(endChar);
        v_bac.push_back(' ');
        isValid = true;
      }
    } else if (tri_bac->isWord) {
      char endChar;
      if (FindFirstWordEnd(tri_for, endChar)) {
        v_for.push_back(endChar);
        v_for.push_back(' ');
        isValid = true;
      }
    }

    if (v_for.size() >= 10 && isValid) {
      std::string str_for(v_for.begin(), v_for.end());
      std::string str_bac(v_bac.begin(), v_bac.end());
      std::reverse(str_bac.begin(), str_bac.end());
      palindromes.insert(str_for + str_bac);
    }
  }
}

//Count approximate tokens in a palindrome string (words + spaces)
int countTokens(const std::string& palindrome) {
  int tokens = 0;
  bool inWord = false;
  for (char c : palindrome) {
    if (c == ' ' || c == '|') {
      if (inWord) {
        tokens++;
        inWord = false;
      }
    } else {
      inWord = true;
    }
  }
  if (inWord) tokens++; // Count final word
  return tokens;
}

//Check if a word is nonsensical based on basic criteria
bool isNonsensicalWord(const std::string& word) {
  if (word.length() < MIN_WORD_LENGTH) return true;
  
  // Filter out common nonsensical patterns
  if (word.length() <= 3) {
    // Check for common nonsensical short words/patterns
    if (word == "SD" || word == "GN" || word == "II" || 
        word == "AA" || word == "EE" || word == "OO" ||
        word == "XX" || word == "ZZ" || word == "EB" ||
        word == "ER" || word == "NI" || word == "PU" ||
        word == "SA" || word == "AT" || word == "REM" ||
        word == "ROC" || word == "SAB" || word == "SUR") {
      return true;
    }
  }
  
  // Filter words that are likely fragmented parts (common in palindromes)
  if (word.length() <= 4) {
    // Common fragmented endings that don't make sense
    if (word == "ASET" || word == "TASET" || word == "SUBSET" ||
        word == "ANDSET" || word == "BESET" || word == "RESET" ||
        word == "INSET" || word == "ONSET" || word == "UPSET" ||
        word == "MERSET" || word == "CORSET" || word == "BASSET" ||
        word == "ASSET" || word == "RUSSET") {
      return true;
    }
  }
  
  // Filter words with too many repeated characters
  int consecutiveCount = 1;
  for (size_t i = 1; i < word.length(); i++) {
    if (word[i] == word[i-1]) {
      consecutiveCount++;
      if (consecutiveCount >= 3) return true; // 3+ consecutive same chars
    } else {
      consecutiveCount = 1;
    }
  }
  
  return false;
}

//Check if a palindrome has basic grammatical structure
bool hasBasicGrammarStructure(const std::string& palindrome) {
  std::vector<std::string> words;
  std::string currentWord;
  
  // Split into words
  for (char c : palindrome) {
    if (c == ' ' || c == '|') {
      if (!currentWord.empty()) {
        words.push_back(currentWord);
        currentWord.clear();
      }
    } else {
      currentWord += c;
    }
  }
  if (!currentWord.empty()) {
    words.push_back(currentWord);
  }
  
  // Filter out palindromes with too many nonsensical words
  int nonsensicalCount = 0;
  for (const std::string& word : words) {
    if (isNonsensicalWord(word)) {
      nonsensicalCount++;
    }
  }
  
  // Reject if more than 20% of words are nonsensical (stricter)
  return words.empty() || (nonsensicalCount * 5 < words.size());
}

//Quality score for palindrome ranking (higher is better)
int calculateQualityScore(const std::string& palindrome) {
  int score = 0;
  
  // Prefer reasonable length (not too short, not too long)
  int length = palindrome.length();
  if (length >= 10 && length <= 50) {
    score += 20;
  } else if (length > 50) {
    score -= (length - 50); // Penalty for being too long
  }
  
  // Count words and spaces for structure assessment
  int wordCount = 0;
  int spaceCount = 0;
  bool inWord = false;
  
  for (char c : palindrome) {
    if (c == ' ') {
      spaceCount++;
      if (inWord) {
        wordCount++;
        inWord = false;
      }
    } else if (c != '|') {
      inWord = true;
    }
  }
  if (inWord) wordCount++;
  
  // Prefer palindromes with 2-5 words per side
  if (wordCount >= 2 && wordCount <= 10) {
    score += 15;
  }
  
  // Bonus for having reasonable word-to-space ratio
  if (spaceCount > 0 && wordCount > 0) {
    score += 10;
  }
  
  // Penalty for consecutive spaces
  if (palindrome.find("  ") != std::string::npos) {
    score -= 10;
  }
  
  return score;
}

//Filter and limit palindromes to stay within token limits
std::set<std::string> filterPalindromes(const std::set<std::string>& palindromes, 
                                       const std::string& startFor, const std::string& startBac, 
                                       bool reverse_search) {
  std::vector<std::pair<int, std::string>> scoredPalindromes;
  
  // Generate full palindromes and score them
  for (const std::string& str : palindromes) {
    std::string fullPalindrome;
    if (reverse_search) {
      const size_t ix = str.find_first_of('|');
      fullPalindrome = "|" + str.substr(ix + 1, str.length() - ix - 1) + startBac + startFor + str.substr(0, ix) + "|";
    } else {
      fullPalindrome = startFor + str + startBac;
    }
    
    // Apply basic grammar filter
    if (hasBasicGrammarStructure(fullPalindrome)) {
      int score = calculateQualityScore(fullPalindrome);
      scoredPalindromes.push_back({score, str});
    }
  }
  
  // Sort by quality score (highest first)
  std::sort(scoredPalindromes.begin(), scoredPalindromes.end(), 
           [](const auto& a, const auto& b) { return a.first > b.first; });
  
  // Select palindromes up to token limit
  std::set<std::string> filteredPalindromes;
  int totalTokens = 0;
  
  for (const auto& scoredPair : scoredPalindromes) {
    const std::string& str = scoredPair.second;
    std::string fullPalindrome;
    if (reverse_search) {
      const size_t ix = str.find_first_of('|');
      fullPalindrome = "|" + str.substr(ix + 1, str.length() - ix - 1) + startBac + startFor + str.substr(0, ix) + "|";
    } else {
      fullPalindrome = startFor + str + startBac;
    }
    
    int tokens = countTokens(fullPalindrome);
    if (totalTokens + tokens <= MAX_TOKENS) {
      filteredPalindromes.insert(str);
      totalTokens += tokens;
    } else {
      break; // Stop if we would exceed token limit
    }
  }
  
  return filteredPalindromes;
}

int main(int argc, const char* argv[]) {
  //Commandline variables
  bool print_help = false;
  bool reverse_search = false;
  bool random_search = false;
  std::string input_text = "|";
  std::string dictionary_path = "dictionary.txt";
  std::string output_path = "palindromes.txt";

  //Commandline setup
  CmdLine cmd("Palindromer");
  cmd.addArgument({ "-h","--help" }, "Print this help message", &print_help);
  cmd.addArgument({ "-d","--dictionary" }, "Path to load the list of valid words", &dictionary_path);
  cmd.addArgument({ "-m","--montecarlo" }, "Use Monte Carlo Search instead of brute-force", &random_search);
  cmd.addArgument({ "-o","--output" }, "Output path for generated palindromes", &output_path);
  cmd.addArgument({ "-r","--reverse" }, "Build palindrome from the inside-out", &reverse_search);
  cmd.addArgument({ "-t","--text" }, "Palindrome input text with '|' as a center divider", &input_text);

  //Parse the command line
  if (!cmd.parse(argc, argv)) {
    return 1;
  }

  //Print the help menu
  if (print_help) {
    cmd.printHelp();
    return 0;
  }

  //Check for the divider
  const size_t firstPipeIx = input_text.find_first_of('|');
  const size_t lastPipeIx = input_text.find_last_of('|');
  if (firstPipeIx == std::string::npos) {
    std::cout << "ERROR: Input next needs at least one '|' divider." << std::endl;
    return 1;
  } else if (firstPipeIx != lastPipeIx) {
    std::cout << "ERROR: Input next must only have one '|' divider." << std::endl;
    return 1;
  }

  //Get starting string from argument
  std::string startFor = input_text.substr(0, firstPipeIx);
  std::string startBac = input_text.substr(firstPipeIx + 1);
  if (reverse_search) { std::swap(startFor, startBac); }

  //Load word frequency list
  std::cout << "Loading Dictionary " << dictionary_path << "..." << std::endl;
  if (!LoadDictionary(dictionary_path)) {
    std::cout << "Failed to load dictionary from file: " << dictionary_path << std::endl;
    return 1;
  }

  //Generate all the palindromes
  std::cout << "Generating..." << std::endl;
  std::set<std::string> palindromes;
  if (random_search) {
    RandSearch(palindromes, startFor, startBac);
  } else {
    BruteSearch(palindromes, startFor, startBac);
  }
  std::cout << "Found " << palindromes.size() << " continuations" << std::endl;

  //Apply filtering to reduce to reasonable token count
  std::cout << "Filtering..." << std::endl;
  std::set<std::string> filteredPalindromes = filterPalindromes(palindromes, startFor, startBac, reverse_search);
  std::cout << "Filtered to " << filteredPalindromes.size() << " high-quality palindromes" << std::endl;

  //Save the results to a text file
  std::cout << "Saving..." << std::endl;
  std::ofstream fout(output_path);
  if (!fout.is_open()) {
    std::cout << "Failed to save results to file: " << output_path << std::endl;
    return 1;
  }
  for (const std::string& str : filteredPalindromes) {
    if (reverse_search) {
      const size_t ix = str.find_first_of('|');
      fout << "|" << str.substr(ix + 1, str.length() - ix - 1) << startBac << startFor << str.substr(0, ix) << "|" << std::endl;
    } else {
      fout << startFor << str << startBac << std::endl;
    }
  }

  //Quit
  std::cout << "Done." << std::endl;
  return 0;
}
