#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>
#include <pthread.h>
#include <sys/sysinfo.h>

#define COLOR_MSG_OK               "\033[01;36mEverything up to date\033[00m\n"
#define COLOR_REPO_PRINTF          "\033[36;1m%-16s\033[00m \033[32;1m%-16s\033[00m\n"
#define COLOR_LANG_PRINTF          "\033[36;1m%-16s\033[00m \033[32;1m%2d\033[00m\n"
#define COLOR_LANG_PRINTFN         "\033[36;1m%-16s\033[00m \033[32;1m%2d\033[00m \033[31;5;1m%2d\033[00m\n"
#define COLOR_LANG_ROW             "├──\033[36;1m%s\033[00m (%d)\n"
#define COLOR_LANG_ROW_LAST        "└──\033[36;1m%s\033[00m (%d)\n"
#define COLOR_REPO_ROW             "│  ├──\033[32;1m%s\033[00m\n"
#define COLOR_REPO_ROW_END         "│  └──\033[32;1m%s\033[00m\n"
#define COLOR_REPO_ROW_LAST        "   ├──\033[32;1m%s\033[00m\n"
#define COLOR_REPO_ROW_LAST_END    "   └──\033[32;1m%s\033[00m\n"
#define COLOR_REPO_ROW_OK          "│  ├──\033[41;1m%s\033[00m\n"
#define COLOR_REPO_ROW_END_OK      "│  └──\033[41;1m%s\033[00m\n"
#define COLOR_REPO_ROW_LAST_OK     "   ├──\033[41;1m%s\033[00m\n"
#define COLOR_REPO_ROW_LAST_END_OK "   └──\033[41;1m%s\033[00m\n"

#define MSG_OK               "Everything up to date\n"
#define REPO_PRINTF          "%-16s %-16s\n"
#define LANG_PRINTF          "%-16s %2d\n"
#define LANG_PRINTFN         "%-16s %2d %2d\n"
#define LANG_ROW             "├──%s (%d)\n"
#define LANG_ROW_LAST        "└──%s (%d)\n"
#define REPO_ROW             "│  ├──%s\n"
#define REPO_ROW_END         "│  └──%s\n"
#define REPO_ROW_LAST        "   ├──%s\n"
#define REPO_ROW_LAST_END    "   └──%s\n"
#define REPO_ROW_OK          "│  ├──%s\n"
#define REPO_ROW_END_OK      "│  └──%s\n"
#define REPO_ROW_LAST_OK     "   ├──%s\n"
#define REPO_ROW_LAST_END_OK "   └──%s\n"

typedef struct repo {
	char name[32];
	char path[256];
	char lang[32];
	uint8_t ok;
} repo_t;

typedef struct lang {
	char name[32];
	repo_t* repos;
	uint8_t not_ok;
	uint8_t len;
} lang_t;

typedef struct lang_f {
	lang_t* languages;
	uint8_t len;
} langf_t;

void prttree(langf_t* l);

void prtlangs(langf_t*);

void add_lang(langf_t*, const char*);

uint8_t contains_folder(const char*, const char*);

void add_repo(langf_t*, const char*, const char*);

void* update_status(void*);

char* repo_root = NULL;

const unsigned char LPRT_F = 0b11000000;
const unsigned char PRT_F = 0b01000000;
unsigned char FLAGS = 0b00000000;
int COLOR_OUT = 1;

int main(int argc, char* argv[]) {
	const int T_COUNT = get_nprocs_conf();
	pthread_t tpool[T_COUNT];

	for (int i1 = 0; i1 < T_COUNT; ++i1) {
		tpool[i1] = 0;
	}

	repo_root = getenv("CODE");

	if (repo_root == NULL) {
		errno = EINVAL;
		fprintf(stderr, "ERROR: Invalid 'CODE' env variable");
	}

	while (*argv != NULL) {
		if (strcmp(*argv, "-l") == 0) {
			FLAGS |= PRT_F;
		} else if (strcmp(*argv, "-ll") == 0) {
			FLAGS |= LPRT_F;
		} else if (strcmp(*argv, "--no-color") == 0) {
			COLOR_OUT = 0;
		}
		argv++;
	}

	DIR* repo_root_dir;
	struct dirent* repo_root_d;
	struct dirent* lang_repo_d;

	langf_t langs = {NULL, 0};

	repo_root_dir = opendir(repo_root);

	if (repo_root_dir == NULL) {
		errno = ENOENT;
		fprintf(stderr, "Unable to open %s", repo_root);
	}

	while ((repo_root_d = readdir(repo_root_dir)) != NULL) {
		if (repo_root_d->d_type == DT_DIR && !strchr(repo_root_d->d_name, '_') && !strchr(repo_root_d->d_name, '.')) {
			add_lang(&langs, repo_root_d->d_name);
		}
	}

	free(repo_root_dir);

	DIR* dir = NULL;
	char* dirname = (char*) calloc(128, sizeof(char));

	for (int i = 0; i < langs.len; ++i) {
		memset(dirname, 0, 128);
		strcpy(dirname, repo_root);
		strcat(dirname, "/");
		strcat(dirname, langs.languages[i].name);
		dir = opendir(dirname);
		if (contains_folder(dirname, ".git")) {
			add_repo(&langs, langs.languages[i].name, dirname);
			free(dir);
			continue;
		}

		while ((lang_repo_d = readdir(dir)) != NULL) {
			if (lang_repo_d->d_type == DT_DIR && !strchr(lang_repo_d->d_name, '.')) {
				char* buf = (char*) calloc(256, sizeof(char));
				strcpy(buf, dirname);
				strcat(buf, "/");
				strcat(buf, lang_repo_d->d_name);
				if (contains_folder(buf, ".git")) {
					add_repo(&langs, langs.languages[i].name, buf);
				}
				free(buf);
			}
		}
		free(dir);
	}

	free(dirname);

	for (int k = 0; k < langs.len; ++k) {
		lang_t* lang = langs.languages + k;
		for (int j = 0; j < lang->len; j++) {
			int n = 0;
			for (n = 0; n < T_COUNT && j < lang->len; ++n, j++) {
				pthread_create(&tpool[n], NULL, update_status, &(lang->repos[j]));
			}
			for (int m = 0; m < T_COUNT && m < n; ++m) {
				pthread_join(tpool[m], NULL);
			}
		}
	}

	if (FLAGS == LPRT_F) {
		prttree(&langs);
	} else {
		prtlangs(&langs);
	}

	for (int l = 0; l < langs.len; ++l) {
		lang_t* lang = langs.languages + l;
		free(lang->repos);
	}
	free(langs.languages);

	return 0;
}

void prtlangs(langf_t* l) {
	uint8_t total = 0;

	char* out = (char*) calloc(1024, sizeof(char));
	char* buf = (char*) calloc(64, sizeof(char));

	for (int k = 0; k < l->len; ++k) {
		lang_t* lang = &(l->languages[k]);
		total += lang->len;
		for (int j = 0; j < lang->len; j++) {
			repo_t* repo = &(lang->repos[j]);
			if (repo->ok == 0) {
				lang->not_ok++;
				if (COLOR_OUT) {
					sprintf(buf, COLOR_REPO_PRINTF, repo->lang, repo->name);
				} else {
					sprintf(buf, REPO_PRINTF, repo->lang, repo->name);
				}
				strcat(out, buf);
				memset(buf, 0, 64);
			}
		}
		if (FLAGS == PRT_F) {
			if (lang->not_ok == 0) {
				if (COLOR_OUT) {
					printf(COLOR_LANG_PRINTF, lang->name, lang->len);
				} else {
					printf(LANG_PRINTF, lang->name, lang->len);
				}
			} else {
				if (COLOR_OUT) {
					printf(COLOR_LANG_PRINTFN, lang->name, lang->len, lang->not_ok);
				} else {
					printf(LANG_PRINTFN, lang->name, lang->len, lang->not_ok);
				}
			}
		}
	}

	if (FLAGS == PRT_F) {
		if (COLOR_OUT) {
			printf(COLOR_LANG_PRINTF, "total", total);
		} else {
			printf(LANG_PRINTF, "total", total);
		}
		printf("\n");
	}
	if (out[0] == '\0') {
		if (COLOR_OUT) {
			fprintf(stdout, COLOR_MSG_OK);
		} else {
			fprintf(stdout, MSG_OK);
		}
	} else {
		fputs(out, stdout);
	}
	free(out);
	free(buf);
}

void prttree(langf_t* l) {
	for (int i = 0; i < l->len; ++i) {
		lang_t* lang = &(l->languages[i]);
		if (i == l->len - 1) {
			if (COLOR_OUT) {
				printf(COLOR_LANG_ROW_LAST, lang->name, lang->len);
			} else {
				printf(LANG_ROW_LAST, lang->name, lang->len);
			}
		} else {
			if (COLOR_OUT) {
				printf(COLOR_LANG_ROW, lang->name, lang->len);
			} else {
				printf(LANG_ROW, lang->name, lang->len);
			}
		}
		for (int j = 0; j < lang->len; ++j) {
			repo_t* repo = &(lang->repos[j]);
			if (i == l->len - 1 && j == lang->len - 1) {
				if (repo->ok) {
					if (COLOR_OUT) {
						printf(COLOR_REPO_ROW_LAST_END, repo->name);
					} else {
						printf(REPO_ROW_LAST_END, repo->name);
					}
				} else {
					if (COLOR_OUT) {
						printf(COLOR_REPO_ROW_LAST_END_OK, repo->name);
					} else {
						printf(REPO_ROW_LAST_END_OK, repo->name);
					}
				}
			} else if (i == l->len - 1) {
				if (repo->ok) {
					if (COLOR_OUT) {
						printf(COLOR_REPO_ROW_LAST, repo->name);
					} else {
						printf(REPO_ROW_LAST, repo->name);
					}
				} else {
					if (COLOR_OUT) {
						printf(COLOR_REPO_ROW_LAST_OK, repo->name);
					} else {
						printf(REPO_ROW_LAST_OK, repo->name);
					}
				}
			} else if (j == lang->len - 1) {
				if (repo->ok) {
					if (COLOR_OUT) {
						printf(COLOR_REPO_ROW_END, repo->name);
					} else {
						printf(REPO_ROW_END, repo->name);
					}
				} else {
					if (COLOR_OUT) {
						printf(COLOR_REPO_ROW_END_OK, repo->name);
					} else {
						printf(REPO_ROW_END_OK, repo->name);
					}
				}
			} else {
				if (repo->ok) {
					if (COLOR_OUT) {
						printf(COLOR_REPO_ROW, repo->name);
					} else {
						printf(REPO_ROW, repo->name);
					}
				} else {
					if (COLOR_OUT) {
						printf(COLOR_REPO_ROW_OK, repo->name);
					} else {
						printf(REPO_ROW_OK, repo->name);
					}
				}
			}
		}
	}
}

void* update_status(void* arg) {
	FILE* fptr;
	repo_t* repo = (repo_t*) arg;
	char buf[128];
	char out[1024];

	memset(out, 0, 1024);
	memset(buf, 0, 128);

	sprintf(buf, "git -C %s status --porcelain", repo->path);

	fptr = popen(buf, "r");

	int n = fread(out, 1, sizeof(out), fptr);

	if (n != 0) {
		repo->ok = 0;
	}

	pclose(fptr);
}

void add_repo(langf_t* l, const char* lang_name, const char* path) {
	for (int i = 0; i < l->len; ++i) {
		if (strcmp(l->languages[i].name, lang_name) == 0) {
			lang_t* lref = &(l->languages[i]);
			lref->len++;

			if (lref->len == 1) {
				lref->repos = (repo_t*) calloc(lref->len, sizeof(repo_t));
			} else {
				repo_t* temp = realloc(lref->repos, lref->len * sizeof(repo_t));
				if (temp == NULL) {
					free(lref->repos);
					errno = ENOMEM;
					exit(1);
				}
				lref->repos = temp;

			}
			repo_t* new_repo = (repo_t*) calloc(1, sizeof(repo_t));

			new_repo->ok = 1;
			strcpy(new_repo->path, path);
			strcpy(new_repo->name, basename((char*) path));
			strcpy(new_repo->lang, lang_name);

			memcpy(lref->repos + lref->len - 1, new_repo, sizeof(repo_t));
			free(new_repo);
		}
	}
}

void add_lang(langf_t* l, const char* lang) {
	l->len++;
	if (l->len == 1 || l->languages == NULL) {
		l->languages = (lang_t*) calloc(l->len, sizeof(lang_t));
	} else {
		lang_t* temp = realloc(l->languages, l->len * sizeof(lang_t));
		if (temp == NULL) {
			free(l->languages);
			errno = ENOMEM;
			exit(1);
		}
		l->languages = temp;
	}

	lang_t* new_lang = (lang_t*) calloc(1, sizeof(lang_t));

	strncpy(new_lang->name, lang, strlen(lang));
	new_lang->len = 0;
	new_lang->not_ok = 0;
	new_lang->repos = NULL;
	memcpy(l->languages + l->len - 1, new_lang, sizeof(lang_t));
	free(new_lang);
}

uint8_t contains_folder(const char* path, const char* folder) {
	struct dirent* sdirent;
	DIR* dir = opendir(path);
	while ((sdirent = readdir(dir)) != NULL) {
		if (sdirent->d_type == DT_DIR && strcmp(sdirent->d_name, folder) == 0) {
			free(dir);
			return 1;
		}
	}

	free(dir);
	return 0;
}
