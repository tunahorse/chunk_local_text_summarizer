#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MAX_SENTENCES 10000
#define MAX_SENTENCE_LENGTH 1000
#define MAX_WORDS 100000
#define MAX_WORD_LENGTH 100
#define MAX_TEXT_LENGTH 1000000  

typedef struct {
    char *word;
    int count;
    double tf_idf;
} WordFreq;



void toLowerCase(char *str);
void tokenizeSentences(char *text, char **sentences, int *sentenceCount);
void tokenizeWords(char *text, char **words, int *wordCount);
int isStopWord(char *word);
void calculateWordFrequencies(char **words, int wordCount, WordFreq **wordFreq, int *uniqueWordCount);
void calculateTFIDF(WordFreq *wordFreq, int uniqueWordCount, int sentenceCount);
void scoreSentences(char **sentences, int sentenceCount, WordFreq *wordFreq, int uniqueWordCount, double *sentenceScores);
void summarize(char **sentences, int sentenceCount, double *sentenceScores, int numSentences, FILE *outputFile);
char* readFile(const char *filename);
void writeFile(const char *filename, char **sentences, int sentenceCount, double *sentenceScores, int numSentences);

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <input_file> <output_file> <summary_length>\n", argv[0]);
        return 1;
    }

    const char *inputFile = argv[1];
    const char *outputFile = argv[2];
    int summaryLength = atoi(argv[3]);

    char *text = readFile(inputFile);
    if (text == NULL) {
        printf("Failed to read input file.\n");
        return 1;
    }

    char **sentences = malloc(MAX_SENTENCES * sizeof(char *));
    int sentenceCount = 0;
    char **words = malloc(MAX_WORDS * sizeof(char *));
    int wordCount = 0;
    WordFreq *wordFreq = NULL;
    int uniqueWordCount = 0;
    double *sentenceScores = calloc(MAX_SENTENCES, sizeof(double));

    tokenizeSentences(text, sentences, &sentenceCount);
    printf("Sentences tokenized. Count: %d\n", sentenceCount);

    tokenizeWords(text, words, &wordCount);
    printf("Words tokenized. Count: %d\n", wordCount);

    calculateWordFrequencies(words, wordCount, &wordFreq, &uniqueWordCount);
    printf("Word frequencies calculated. Unique words: %d\n", uniqueWordCount);

    calculateTFIDF(wordFreq, uniqueWordCount, sentenceCount);
    printf("TF-IDF calculated.\n");

    scoreSentences(sentences, sentenceCount, wordFreq, uniqueWordCount, sentenceScores);
    printf("Sentences scored.\n");

    writeFile(outputFile, sentences, sentenceCount, sentenceScores, summaryLength);
    printf("Summary written to %s\n", outputFile);

    // Free allocated memory
    free(text);
    for (int i = 0; i < sentenceCount; i++) {
        free(sentences[i]);
    }
    free(sentences);
    for (int i = 0; i < wordCount; i++) {
        free(words[i]);
    }
    free(words);
    for (int i = 0; i < uniqueWordCount; i++) {
        free(wordFreq[i].word);
    }
    free(wordFreq);
    free(sentenceScores);

    return 0;
}

void toLowerCase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

void tokenizeSentences(char *text, char **sentences, int *sentenceCount) {
    char *saveptr;
    char *sentence = strtok_r(text, ".!?", &saveptr);
    while (sentence != NULL && *sentenceCount < MAX_SENTENCES) {
        while (*sentence == ' ' || *sentence == '\n' || *sentence == '\t') {
            sentence++;
        }
        sentences[*sentenceCount] = strdup(sentence);
        (*sentenceCount)++;
        sentence = strtok_r(NULL, ".!?", &saveptr);
    }
}

void tokenizeWords(char *text, char **words, int *wordCount) {
    char *saveptr;
    char *word = strtok_r(text, " \t\n\r\f\v,.-!?()[]{}:;\"'", &saveptr);
    while (word != NULL && *wordCount < MAX_WORDS) {
        toLowerCase(word);
        if (strlen(word) > 0 && !isStopWord(word)) {
            words[*wordCount] = strdup(word);
            (*wordCount)++;
        }
        word = strtok_r(NULL, " \t\n\r\f\v,.-!?()[]{}:;\"'", &saveptr);
    }
}

int isStopWord(char *word) {
    char *stopWords[] = {"the", "a", "an", "and", "or", "but", "in", "on", "at", "to", "for", "of", "with", "by", "from", "up", "about", "into", "over", "after"};
    int stopWordCount = sizeof(stopWords) / sizeof(stopWords[0]);
    
    for (int i = 0; i < stopWordCount; i++) {
        if (strcmp(word, stopWords[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

void calculateWordFrequencies(char **words, int wordCount, WordFreq **wordFreq, int *uniqueWordCount) {
    *wordFreq = malloc(wordCount * sizeof(WordFreq));
    *uniqueWordCount = 0;

    for (int i = 0; i < wordCount; i++) {
        int found = 0;
        for (int j = 0; j < *uniqueWordCount; j++) {
            if (strcmp(words[i], (*wordFreq)[j].word) == 0) {
                (*wordFreq)[j].count++;
                found = 1;
                break;
            }
        }
        if (!found) {
            (*wordFreq)[*uniqueWordCount].word = strdup(words[i]);
            (*wordFreq)[*uniqueWordCount].count = 1;
            (*wordFreq)[*uniqueWordCount].tf_idf = 0.0;
            (*uniqueWordCount)++;
        }
    }
}

void calculateTFIDF(WordFreq *wordFreq, int uniqueWordCount, int sentenceCount) {
    for (int i = 0; i < uniqueWordCount; i++) {
        double tf = (double)wordFreq[i].count / sentenceCount;
        double idf = log((double)sentenceCount / wordFreq[i].count);
        wordFreq[i].tf_idf = tf * idf;
    }
}

void scoreSentences(char **sentences, int sentenceCount, WordFreq *wordFreq, int uniqueWordCount, double *sentenceScores) {
    for (int i = 0; i < sentenceCount; i++) {
        char *sentence = strdup(sentences[i]);
        toLowerCase(sentence);
        
        char *word;
        char *saveptr;
        word = strtok_r(sentence, " \t\n\r\f\v,.-!?()[]{}:;\"'", &saveptr);
        
        while (word != NULL) {
            for (int j = 0; j < uniqueWordCount; j++) {
                if (strcmp(word, wordFreq[j].word) == 0) {
                    sentenceScores[i] += wordFreq[j].tf_idf;
                    break;
                }
            }
            word = strtok_r(NULL, " \t\n\r\f\v,.-!?()[]{}:;\"'", &saveptr);
        }
        
        free(sentence);
    }
}

char* readFile(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file: %s\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(length + 1);
    if (buffer == NULL) {
        printf("Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    size_t bytesRead = fread(buffer, 1, length, file);
    if (bytesRead < length) {
        printf("Error reading file\n");
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[length] = '\0';
    fclose(file);
    return buffer;
}

void writeFile(const char *filename, char **sentences, int sentenceCount, double *sentenceScores, int numSentences) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error opening output file: %s\n", filename);
        return;
    }

    fprintf(file, "Summary:\n\n");
    summarize(sentences, sentenceCount, sentenceScores, numSentences, file);

    fclose(file);
}

void summarize(char **sentences, int sentenceCount, double *sentenceScores, int numSentences, FILE *outputFile) {
    int *topSentences = (int *)malloc(numSentences * sizeof(int));
    for (int i = 0; i < numSentences; i++) {
        topSentences[i] = -1;
        double maxScore = -1;
        for (int j = 0; j < sentenceCount; j++) {
            if (sentenceScores[j] > maxScore) {
                int alreadySelected = 0;
                for (int k = 0; k < i; k++) {
                    if (topSentences[k] == j) {
                        alreadySelected = 1;
                        break;
                    }
                }
                if (!alreadySelected) {
                    maxScore = sentenceScores[j];
                    topSentences[i] = j;
                }
            }
        }
    }

    for (int i = 0; i < numSentences; i++) {
        if (topSentences[i] != -1) {
            fprintf(outputFile, "%s.\n\n", sentences[topSentences[i]]);
        }
    }

    free(topSentences);
}
