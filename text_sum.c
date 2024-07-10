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
    char *content;
    double score;
    int index;
} Sentence;

void tokenizeSentences(char *text, Sentence **sentences, int *sentenceCount);
void tokenizeWords(char *sentence, char **words, int *wordCount);
int isStopWord(char *word);
double calculateSimilarity(char *sentence1, char *sentence2);
void textRank(Sentence **sentences, int sentenceCount, int iterations);
int compareSentences(const void *a, const void *b);
void summarize(Sentence **sentences, int sentenceCount, int numSentences, FILE *outputFile);
char* readFile(const char *filename);
void writeFile(const char *filename, Sentence **sentences, int sentenceCount, int numSentences);

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <input_file> <output_file> <summary_percentage>\n", argv[0]);
        return 1;
    }

    const char *inputFile = argv[1];
    const char *outputFile = argv[2];
    double summaryPercentage = atof(argv[3]);

    char *text = readFile(inputFile);
    if (text == NULL) {
        printf("Failed to read input file.\n");
        return 1;
    }

    Sentence *sentences[MAX_SENTENCES];
    int sentenceCount = 0;

    tokenizeSentences(text, sentences, &sentenceCount);
    int numSentences = ceil(sentenceCount * summaryPercentage / 100.0);

    textRank(sentences, sentenceCount, 20);  // 20 iterations for TextRank

    writeFile(outputFile, sentences, sentenceCount, numSentences);

    printf("Summary written to %s\n", outputFile);
    printf("Total sentences in summary: %d\n", numSentences);

    // Free allocated memory
    free(text);
    for (int i = 0; i < sentenceCount; i++) {
        free(sentences[i]->content);
        free(sentences[i]);
    }

    return 0;
}

void tokenizeSentences(char *text, Sentence **sentences, int *sentenceCount) {
    char *start = text;
    char *end;
    while (*start && *sentenceCount < MAX_SENTENCES) {
        // Find the end of the sentence
        end = strpbrk(start, ".!?");
        if (end == NULL) {
            break;  // No more sentences
        }
        
        // Allocate memory for the sentence
        int length = end - start + 1;
        char *sentenceContent = malloc(length + 1);
        strncpy(sentenceContent, start, length);
        sentenceContent[length] = '\0';
        
        // Create the Sentence struct
        sentences[*sentenceCount] = malloc(sizeof(Sentence));
        sentences[*sentenceCount]->content = sentenceContent;
        sentences[*sentenceCount]->score = 1.0;  // Initial score
        sentences[*sentenceCount]->index = *sentenceCount;
        
        (*sentenceCount)++;
        
        // Move to the start of the next sentence
        start = end + 1;
        while (*start && isspace((unsigned char)*start)) {
            start++;
        }
    }
}

void tokenizeWords(char *sentence, char **words, int *wordCount) {
    char *word = strtok(sentence, " \t\n\r\f\v,.-!?()[]{}:;\"'");
    while (word != NULL && *wordCount < MAX_WORDS) {
        for (int i = 0; word[i]; i++) {
            word[i] = tolower(word[i]);
        }
        if (strlen(word) > 0 && !isStopWord(word)) {
            words[*wordCount] = strdup(word);
            (*wordCount)++;
        }
        word = strtok(NULL, " \t\n\r\f\v,.-!?()[]{}:;\"'");
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

double calculateSimilarity(char *sentence1, char *sentence2) {
    char *words1[MAX_WORDS];
    char *words2[MAX_WORDS];
    int wordCount1 = 0, wordCount2 = 0;
    
    char *sent1Copy = strdup(sentence1);
    char *sent2Copy = strdup(sentence2);
    
    tokenizeWords(sent1Copy, words1, &wordCount1);
    tokenizeWords(sent2Copy, words2, &wordCount2);

    int commonWords = 0;
    for (int i = 0; i < wordCount1; i++) {
        for (int j = 0; j < wordCount2; j++) {
            if (strcmp(words1[i], words2[j]) == 0) {
                commonWords++;
                break;
            }
        }
    }

    double similarity = (double)commonWords / (log(wordCount1 + 1) + log(wordCount2 + 1));

    for (int i = 0; i < wordCount1; i++) free(words1[i]);
    for (int i = 0; i < wordCount2; i++) free(words2[i]);
    free(sent1Copy);
    free(sent2Copy);

    return similarity;
}

void textRank(Sentence **sentences, int sentenceCount, int iterations) {
    double d = 0.85;  // Damping factor

    for (int iter = 0; iter < iterations; iter++) {
        for (int i = 0; i < sentenceCount; i++) {
            double score = 1 - d;
            for (int j = 0; j < sentenceCount; j++) {
                if (i != j) {
                    double similarity = calculateSimilarity(sentences[i]->content, sentences[j]->content);
                    score += d * similarity * sentences[j]->score;
                }
            }
            sentences[i]->score = score;
        }
    }
}

int compareSentences(const void *a, const void *b) {
    Sentence *s1 = *(Sentence **)a;
    Sentence *s2 = *(Sentence **)b;
    if (s1->score > s2->score) return -1;
    if (s1->score < s2->score) return 1;
    return s1->index - s2->index;  // Maintain original order for equal scores
}

void summarize(Sentence **sentences, int sentenceCount, int numSentences, FILE *outputFile) {
    qsort(sentences, sentenceCount, sizeof(Sentence*), compareSentences);
    
    // Select top sentences
    Sentence *selectedSentences[MAX_SENTENCES];
    for (int i = 0; i < numSentences && i < sentenceCount; i++) {
        selectedSentences[i] = sentences[i];
    }

    // Sort selected sentences by their original index
    qsort(selectedSentences, numSentences, sizeof(Sentence*), compareSentences);

    // Output sentences in original order
    for (int i = 0; i < numSentences && i < sentenceCount; i++) {
        fprintf(outputFile, "%s\n", selectedSentences[i]->content);
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

void writeFile(const char *filename, Sentence **sentences, int sentenceCount, int numSentences) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error opening output file: %s\n", filename);
        return;
    }

    fprintf(file, "Summary:\n\n");
    summarize(sentences, sentenceCount, numSentences, file);

    fclose(file);
}
