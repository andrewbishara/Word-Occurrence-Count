#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>


#define PATHSIZE 1024
#define INITIAL_BUF_SIZE 8
#define BUF_SIZE 1024

#define DEBUG 0

typedef struct WordCount {
    char *word;
    int count;
    struct WordCount *next;
} WordCount;

char *my_strdup(const char *s) {
    if (s == NULL) {
        return NULL;
    }
    char *d = malloc(strlen(s) + 1);
    if (d == NULL) {
        return NULL;
    }
    strcpy(d, s);
    return d;
}


WordCount *head = NULL;

int compare_counts(const void *a, const void *b) {
    WordCount *wc1 = *(WordCount **)a;
    WordCount *wc2 = *(WordCount **)b;

    if (wc1->count > wc2->count) return -1; // Negative if wc1 should come first
    if (wc1->count < wc2->count) return 1;  // Positive if wc2 should come first

    return strcmp(wc1->word, wc2->word);
}

void add_word_count(char *word);
void print_word_counts(const char *filename);

int is_char(char c) {
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

int isHyphen(char c){
    if(c == '-') return 1;
    else return 0;
}

int is_word_char(char c, int position, int in_word) {
    if (isdigit(c)) return 0;
    return isalpha((unsigned char)c) || (c == '\'' && in_word);
}

void count(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror(path);
        exit(EXIT_FAILURE);
    }

    char *buf = malloc(BUF_SIZE);
    if (!buf) {
        perror("malloc");
        close(fd);
        exit(EXIT_FAILURE);
    }
    size_t buf_size = BUF_SIZE;
    size_t index = 0;
    int in_word = 0;
    char *word_start = NULL;

    ssize_t bytes;
    // while there are still bytes in the file
    while ((bytes = read(fd, buf + index, buf_size - index - 1)) > 0) {
        buf[index + bytes] = '\0';

        // iterate through buffer
        for (size_t i = index; i < index + bytes; ++i) {
            int position = word_start ? (int)(i - (word_start - buf)) : 0;
            // If the char counts as the start of a word
            if (is_word_char(buf[i], position, in_word)) {
                if (!in_word) {
                    in_word = 1; 
                    word_start = &buf[i];
                }
            } else {
                if (in_word) {
                    // If char is a hypen and the next char is a letter
                    if (isHyphen(buf[i]) && i + 1 < index + bytes && is_char(buf[i + 1])) {
                        // If previous char is also a letter
                        if((isHyphen(buf[i]) && is_char(buf[i - 1]))) continue;
                        // Otherwise, this hypen does not count, end of the word
                        else{
                            in_word = 0;
                            buf[i] = '\0'; 
                            add_word_count(word_start);
                            word_start = NULL; 
                        }
                    // End of the word
                    } else {
                        in_word = 0;
                        buf[i] = '\0'; 
                        add_word_count(word_start);
                        word_start = NULL;
                    }
                }
            }
        }

        
        if (in_word && index + bytes >= buf_size - 1) {
            buf[buf_size - 1] = '\0';
            add_word_count(word_start);
            in_word = 0;
            word_start = NULL;
        }

        if (buf_size - index <= (size_t)bytes) {
            buf_size *= 2;
            char *new_buf = realloc(buf, buf_size);
            if (!new_buf) {
                perror("realloc");
                free(buf);
                close(fd);
                exit(EXIT_FAILURE);
            }
            buf = new_buf;
        }

        index += bytes;
    }

    if (in_word) {
        buf[index] = '\0'; 
        add_word_count(word_start);
    }

    if (bytes == -1) {
        perror("read");
        free(buf);
        close(fd);
        exit(EXIT_FAILURE);
    }

    free(buf);
    close(fd);
}

void traverse(const char *path) {
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    char newPath[PATHSIZE];

    if ((dir = opendir(path)) == NULL) {
        perror("opendir");
        return;
    }

    // Loop through directory and its subdirectories
    while ((entry = readdir(dir)) != NULL) {
        if(DEBUG) printf("Going into directory: %s.\n", entry->d_name);
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Add filename to end of path
        snprintf(newPath, PATHSIZE, "%s/%s", path, entry->d_name);

        // Check file type, then deal with it
        if (stat(newPath, &statbuf) == 0) {
            if (S_ISREG(statbuf.st_mode)) {
                char *ext = strrchr(entry->d_name, '.');
                //If its a .txt file
                if (ext && strcmp(ext, ".txt") == 0) {
                    if(DEBUG) printf("Beginning to count file: %s.\n", entry->d_name);
                    count(newPath);
                }
            // If its a directory
            } else if (S_ISDIR(statbuf.st_mode)) {
                traverse(newPath);
            }
        } else {
            perror(newPath);
        }
    }
    closedir(dir);
}

void add_word_count(char *word) {
    for (char *p = word; *p; ++p) *p = tolower(*p);

    for (WordCount *wc = head; wc != NULL; wc = wc->next) {
        if (strcmp(wc->word, word) == 0) {
            wc->count++;
            return;
        }
    }
	
	WordCount *newWordCount = malloc(sizeof(WordCount));
	if (!newWordCount) {
    		perror("malloc");
    		exit(EXIT_FAILURE);
	}

	newWordCount->word = my_strdup(word);
	if (!newWordCount->word) {
    		free(newWordCount);
    		perror("my_strdup");
    		exit(EXIT_FAILURE);
	}

newWordCount->count = 1;
newWordCount->next = head;
head = newWordCount;

}

void print_word_counts(const char *filename) {
    int size = 0;
    for (WordCount *wc = head; wc != NULL; wc = wc->next) {
        size++;
    }
    WordCount **array = malloc(size * sizeof(WordCount*));
    if (!array) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    int i = 0;
    for (WordCount *wc = head; wc != NULL; wc = wc->next) {
        array[i++] = wc;
    }

    qsort(array, size, sizeof(WordCount*), compare_counts);

    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("fopen");
        free(array);
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < size; i++) {
        fprintf(file, "%s %d\n", array[i]->word, array[i]->count);
    }

    fclose(file);
    free(array);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "No arguments passed! Usage: %s <directory_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct stat statbuf;
    for(int i = 1; i <argc; i++){
        const char *path = argv[i];

        if(stat(path, &statbuf) == 0){
            if(S_ISREG(statbuf.st_mode)){
                if(DEBUG) printf("Opening text file at: %s\n", path);
                count(argv[i]);
            }
            else if(S_ISDIR(statbuf.st_mode)){
                if(DEBUG) printf("Starting directory traversal at: %s\n", path);
		    
                traverse(path);

                if(DEBUG) printf("Finished directory traversal.\n");
            }


        }
    }

    if(DEBUG) printf("Finished iterating through arguments.\n");
    if (head == NULL) {
        printf("No words counted.\n");
        } else {
        printf("Printing word counts to file.\n");
        print_word_counts("word_counts.txt");
        }
        WordCount *current = head;
        while (current != NULL) {
            WordCount *temp = current;
            current = current->next;
            free(temp->word);
            free(temp);
        }

    printf("Word counting and cleanup completed.\n");

    return EXIT_SUCCESS;
}