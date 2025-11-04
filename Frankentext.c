#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_WORD_COUNT 15'000
#define MAX_SUCCESSOR_COUNT (MAX_WORD_COUNT / 2)

char book[] = {
#embed "pg84.txt" /// Stores the content of the file as an array of chars.
    , '\0'};      /// Makes `book` a string.

/// Array of tokens registered so far.
/// No duplicates are allowed.
char *tokens[MAX_WORD_COUNT];
/// `tokens`'s current size
size_t tokens_size = 0;

/// Array of successor tokens
/// One token can have many successor tokens. `succs[x]` corresponds to
/// `token[x]`'s successors.
char *succs[MAX_WORD_COUNT][MAX_SUCCESSOR_COUNT];
/// `succs`'s current size
size_t succs_sizes[MAX_WORD_COUNT];

/// Overwrites non-printable characters in `book` with a space.
/// Non-printable characters may lead to duplicates like
/// `"\xefthe" and "the"` even both print `the`.
void replace_non_printable_chars_with_space() {
  for (size_t i = 0; book[i] != '\0'; ++i) {
    unsigned char c = static_cast<unsigned char>(book[i]);
    if (!isprint(c) && c != '\n' && c != '\r' && c != '\t') {
      book[i] = ' ';
    }
  }
}

size_t token_id(char *token) {
  size_t id;
  for (id = 0; id < tokens_size; ++id) {
    if (strcmp(tokens[id], token) == 0) {
      return id;
    }
  }
  tokens[id] = token;
  ++tokens_size;
  return id;
}

/// Appends the token \c succ to the successors list of \c token.
void append_to_succs(char *token, char *succ) {
  auto next_empty_index_ptr = &succs_sizes[token_id(token)];

  if (*next_empty_index_ptr >= MAX_SUCCESSOR_COUNT) {
    printf("Successor array full.");
    exit(EXIT_FAILURE);
  }

  succs[token_id(token)][(*next_empty_index_ptr)++] = succ;
}

/// Creates tokens on \c book and fills \c tokens and \c succs using
/// the functions \c token_id and \c append_to_succs.
void tokenize_and_fill_succs(char *delimiters, char *str) {
  // strtok modifies `str` in place and returns pointers into `book`
  char *prev = nullptr;
  char *tok = strtok(str, delimiters);
  while (tok) {
    token_id(tok); // ensure token is registered
    if (prev) {
      append_to_succs(prev, tok);
    }
    prev = tok;
    tok = strtok(nullptr, delimiters);
  }
}

/// Returns last character of a string
char last_char(char *str) {
  size_t n = strlen(str);
  if (n == 0) return '\0';
  return str[n - 1];
}

/// Returns whether the token ends with `!`, `?` or `.`.
bool token_ends_a_sentence(char *token) {
  char c = last_char(token);
  return c == '!' || c == '?' || c == '.';
}

/// Returns a random `token_id` that corresponds to a `token` that starts with a
/// capital letter.
/// Uses \c tokens and \c tokens_size.
size_t random_token_id_that_starts_a_sentence() {
  if (tokens_size == 0) {
    fprintf(stderr, "No tokens available.\n");
    exit(EXIT_FAILURE);
  }

  // Try random sampling for a while
  for (int tries = 0; tries < 10000; ++tries) {
    size_t id = static_cast<size_t>(rand()) % tokens_size;
    unsigned char c = static_cast<unsigned char>(tokens[id][0]);
    if (isupper(c)) return id;
  }

  // Fallback: scan linearly for any capitalized start
  for (size_t id = 0; id < tokens_size; ++id) {
    unsigned char c = static_cast<unsigned char>(tokens[id][0]);
    if (isupper(c)) return id;
  }

  // If nothing capitalized exists, just return a random one
  return static_cast<size_t>(rand()) % tokens_size;
}

/// Generates a random sentence using \c tokens, \c succs, and \c succs_sizes.
char *generate_sentence(char *sentence, size_t sentence_size) {
  size_t current_token_id = random_token_id_that_starts_a_sentence();
  auto token = tokens[current_token_id];

  sentence[0] = '\0';
  strncat(sentence, token, sentence_size - 1);
  if (token_ends_a_sentence(token))
    return sentence;

  size_t sentence_len_next;

  do {
    size_t succ_count = succs_sizes[current_token_id];
    if (succ_count == 0) break;

    size_t next_index = static_cast<size_t>(rand()) % succ_count;
    char *next_tok = succs[current_token_id][next_index];

    // +1 for the space we add before the next token
    sentence_len_next = strlen(sentence) + 1 + strlen(next_tok);
    if (sentence_len_next >= sentence_size - 1) break;

    strcat(sentence, " ");
    strcat(sentence, next_tok);

    current_token_id = token_id(next_tok);
    token = next_tok;
    if (token_ends_a_sentence(token)) break;

  } while (sentence_len_next < sentence_size - 1);

  return sentence;
}

int main() {
  replace_non_printable_chars_with_space();

  char *delimiters = (char *)" \n\r\t";
  tokenize_and_fill_succs(delimiters, book);

  char sentence[1000];
  srand(time(nullptr)); // Be random each time we run the program

  // Generate sentences until we find a question sentence.
  do {
    generate_sentence(sentence, sizeof sentence);
  } while (last_char(sentence) != '?');
  puts(sentence);
  puts("");

  // Initialize `sentence` and then generate sentences until we find a sentence
  // ending with an exclamation mark.
  do {
    generate_sentence(sentence, sizeof sentence);
  } while (last_char(sentence) != '!');
  puts(sentence);
}
