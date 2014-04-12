//      snusp.c
//      
//      Copyright 2011 linkboss <linkboss@gmail.com>
//      
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either version 2 of the License, or
//      (at your option) any later version.
//      
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//      
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//      MA 02110-1301, USA.

#include <stdlib.h>
#include <stdio.h>
#define MEMORY_SIZE 32768

typedef struct context_s
{
	int x;
	int y;
	int or;
	struct context_s *prev;
} context;

int execStep(char command, char *memory, int *mp, context **currentContext);

int main(int argc, char **argv)
{
	char *filename = argv[argc-1];
	//char *filename = "test.snu";
	int  i, j, w, h, ret;
	int px, py; //Pointeurs d'instruction (x/y), orientation
	int mp; //Pointeur mémoire
	int stop = 0;
	char debug = 0;
	char **program;
	char *memory;
	context *currentContext;
	
	//Vérification de l'existence d'un ficher en entrée
	if(argc == 1)
	{
		printf("Usage : snusp <file>\n");
		return 1;
	}
	
	//Lecture de la taille du programme et allocation de la mémoire
	getProgramSize(filename, &w, &h);
	program = malloc(h * sizeof(char*));
	for(i = 0; i < h; i++)
		program[i] = malloc(w * sizeof(char));
	
	//Chargement du programme depuis le fichier
	loadProgram(filename, program, w, h);
	
	//Lecture du point de départ du programme et erreur si non trouvé.
	ret = getStart(program, w, h, &px, &py);
	if(ret == 1)
	{
		printf("Error : no usable char found.\n");
		free(program);
		return 1;
	}
	
	//Si ret = 2, on active le mode debug
	if(ret == 2)
		debug = 1;
	
	//Initialisation de la mémoire (32 Ko)
	memory = malloc(MEMORY_SIZE * sizeof(char));
	
	//Création du contexte de base
	currentContext = malloc(sizeof(context));
	currentContext->x = px;
	currentContext->y = py;
	currentContext->or = 8;
	currentContext->prev = NULL;
	
	//Initialisation du memory pointer
	mp = 0;
	ret = 0;
	
	//Exécution du programme
	while(!stop)
	{
		ret = execStep(program[currentContext->y][currentContext->x], memory, &mp, &currentContext);
		
		
		movePointer(currentContext);
		
		//On stoppe l'exécution si il sort du fichier
		if(currentContext->x == w || currentContext->y == h || currentContext->x < 0 || currentContext->y < 0 || ret == -1)
			stop = 1;
		
		
		if(debug)
		{
			//Affichage de celui-ci (à supprimer)
			for(j = 0; j < h;j++)
			{
				for (i = 0; i < w; i++)
				{
					if(j == currentContext->y && i == currentContext->x)
						printf("0");
					else
						printf("%c", program[j][i]);
				}
				printf("\n");
			}
			#ifdef WIN32
				system('cls');
			#else
				printf("\033[H\033[2J");
			#endif
			usleep(25000);
		}
	}
	printf("\n");
	return 0;
}

//Crée un contexte
int createContext(context **oldContext)
{
	context *currentContext;
	
	currentContext = malloc(sizeof(context));
	if(!currentContext)
		exit(EXIT_FAILURE);
	currentContext->x = (*oldContext)->x;
	currentContext->y = (*oldContext)->y;
	currentContext->or = (*oldContext)->or;
	
	currentContext->prev = *oldContext;
	*oldContext = currentContext;
	
	return 0;
}

int deleteContext(context **currentContext)
{
	context *prevContext;
	
	if(!(*currentContext)->prev)
		return 1;
	prevContext = (*currentContext)->prev;
	free(*currentContext);
	*currentContext = prevContext;
	movePointer(*currentContext);
	return 0;
}

void clean_stdin(void)
{
    int c;
    
    do {
        c = getchar();
    } while (c != '\n' && c != EOF);
}

int execStep(char command, char *memory, int *mp, context **currentContext)
{
	switch(command)
	{
		case '+':
			memory[*mp]++;
			break;
		case '-':
			memory[*mp]--;
			break;
		case '>':
			(*mp)++;
			break;
		case '<':
			(*mp)--;
			break;
		case ',':
			memory[*mp] = getchar();
			clean_stdin();
			break;
		case '.':
			printf("%c", memory[*mp]);
			break;
		case '!':
			movePointer(*currentContext);
			break;
		case '?':
			if(memory[*mp] == 0)
				movePointer(*currentContext);
			break;
		case '/':
			changeOrientation(*currentContext, 1);
			break;
		case '\\':
			changeOrientation(*currentContext, 2);
			break;
		case '@':
			createContext(currentContext);
			break;
		case '#':
			if(deleteContext(currentContext))
				return -1;
			break;
		case '^':
			while(!getchar());
			clean_stdin();
			break;
	}
	
	return 0;
}

int changeOrientation(context *currentContext, int type)
{
	switch(currentContext->or)
	{
		case 1:
			if(type == 1)
				currentContext->or = 2;
			else
				currentContext->or = 8;
			break;
		case 2:
			if(type == 1)
				currentContext->or = 1;
			else
				currentContext->or = 4;
			break;
		case 4:
			if(type == 1)
				currentContext->or = 8;
			else
				currentContext->or = 2;
			break;
		case 8:
			if(type == 1)
				currentContext->or = 4;
			else
				currentContext->or = 1;
			break;
	}
	
	return 0;
}

int movePointer(context *currentContext)
{
	switch(currentContext->or)
	{
		case 8:
			(currentContext->x)++;
			break;
		case 2:
			(currentContext->x)--;
			break;
		case 1:
			(currentContext->y)++;
			break;
		case 4:
			(currentContext->y)--;
			break;
	}
	
	return 0;
}

//Lecture de la taille du programme
int getProgramSize(char *filename, int *w, int *h)
{
	FILE *fp = fopen(filename, "r");
	int i, j, k, c;
	
	i = 0;
	j = 0;
	k = 0;
	
	while(c != EOF)
	{
		c = fgetc(fp);
		if(c == '\n')
		{
			j++;
			i = -1;
			
		}
		
		i++;
		if(k < i)
			k = i;
	}
	
	*w = k;
	*h = j;
	fclose(fp);
	
	return 0;
}

//Chargement du programme en mémoire
int loadProgram(char *filename, char **program, int w, int h)
{
	FILE *fp = fopen(filename, "r");
	int i, j, k, c;
	
	for(j = 0; j < h;j++)
	{
		
		for (i = 0; i < w; i++)
		{
			c = fgetc(fp);
			
			if(c == '\n')
			{
				for(k = i+1; k < w; k++) //On comble les vides
					program[j][k] = ' ';
				i = w;
			}
			else if(c == '_')
				return 0;
			else
				program[j][i] = (char)c;
		}
		
		//Si on est arrivé au bout de la boucle (i = w), on saute l'\n
		if(i == w)
			fgetc(fp);
	}
	
	fclose(fp);
	
	return 0;
}

//Récupère l'endroit de départ du programme (i.e. le *, ou le caractère le plus en haut à gauche)
int getStart(char **program, int x, int y, int *sx, int *sy)
{
	int fx, fy, first;
	int i, j;
	char c;
	
	for(j = 0; j < y;j++)
	{
		for (i = 0; i < x; i++)
		{
			c = program[j][i];
			
			
			if(c == '$' || c == '~')
			{
				*sx = i+1;
				*sy = j;
				if(c == '~')
					return 2;
				else
					return 0;
			}
			
			//Recherche du premier caractère exécutable s'il n'a pas été rencontré
			if(!first)
			{
				if(c == '>' || c == '<' || c == '+' || c == '-' || c == ',' || c == '.' || c == '/' || c == '\\' || c == '?' || c == '!')
				{
					first = 1;
					fx = i;
					fy = j;
				}
			}
		}
	}
	
	if(first)
	{
		*sx = fx;
		*sy = fy;
		return 0;
	}
	
	return 1;
}
