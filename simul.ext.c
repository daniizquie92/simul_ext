#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include "cabeceras.h"

#define LONGITUD_COMANDO 100

// DECLARACION DE LAS FUNCIONES 
void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps);
int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2);
void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup);
int BuscaFich(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombre);
void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos);
int Renombrar(EXT_ENTRADA_DIR *directorio, char *nombreantiguo, char *nombrenuevo);
int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre);
int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos,EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock,char *nombre); //FILE *fich
int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos,EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock,EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino,  FILE *fich);
void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich);
void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich);
void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich);
void GrabarDatos(EXT_DATOS *memdatos, FILE *fich);

//CON ESTA FUNCION COMPROBAREMOS QUE SEA VALIDO EL COMANDO INTRODUCIDO Y SI LO ES, ASIGNAREMOS A ORDEN, ARGUMENTO1 Y ARGUMENTO2 SUS CORRESPONDIENTES VALORES
//LA FORMA DE FUNCIONAR ES LA SIGUIENTE: AGRUPAMOS LOS COMANDOS SEGUN SU NUMERO DE ARGUMENTOS, 1,3,2 RESPECTIVAMENTE
//CON UN STRTOK DIVIDIMOS LA CADENA Y DEPENDIENDO DEL NUMERO DE ARGUMENTOS QUE DEBIERA TENER LA CADENA LO HAREMOS HASTA 4 VECES (de forma que la 4 si no devuelve NULL estaria mal)
//ALMACENAMOS CADA VALOR EN SU CORRESPONDIENTE VARIABLE
//DEVOLVEMOS 0 o 1 DEPENDIENDO SI ES VALIDA O NO
int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2){
	char* token;
	
	token=strtok(strcomando, " \n"); //primer elemento del comando
	strcpy(orden,token);
	//Comprobamos si ha introducido el comando "info" correctamente
	if(strcmp(orden,"info")==0||strcmp(orden,"bytemaps")==0||strcmp(orden,"dir")==0||strcmp(token,"salir")==0){
		token=strtok(NULL, " \n");
		if(token!=NULL){
			printf("Numero de parametros erroneo\n");
			return 1;
		}
		else
			return 0;
	}

	//Comprobamos si ha introducido el comando "rename" correctamente
	else if(strcmp(token,"rename")==0||strcmp(token,"copy")==0){
		token=strtok(NULL, " \n");
		if(token==NULL){
			printf("Numero de parametros erroneo\n");
			return 1;
		}
		strcpy(argumento1,token);

		token=strtok(NULL, " \n");
		if(token==NULL){
			printf("Numero de parametros erroneo\n");
			return 1;
		}
		strcpy(argumento2,token);
		
		token=strtok(NULL, " \n");
		if(token!=NULL){
			printf("Numero de parametros erroneo\n");
			return 1;
		}		
		else
			return 0;		
	}
		//Comprobamos si ha introducido el comando "imprimir" correctamente
	else if(strcmp(token,"imprimir")==0||strcmp(token,"remove")==0){
		token=strtok(NULL, " \n");
		if(token==NULL){
			printf("Numero de parametros erroneo\n");
			return 1;
		}
		strcpy(argumento1,token);
		
		token=strtok(NULL, " \n");
		if(token!=NULL){
			printf("Numero de parametros erroneo\n");
			return 1;
		}
		else
			return 0;
	}
	else{
		printf("ERROR: comando ilegal [bytemaps, copy, dir, info, imprimir, rename, remove, salir]\n");
		return 1;
	}
}

//ESTA FUNCION SERÁ PARA CUANDO EL USUARIO ESCRIBE INFO
//IMPRIMIMOS CADA VALOR DEL SUPERBLOQUE
void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup){
	printf("Bloque %d Bytes\n", psup->s_block_size);
	printf("Inodos particion = %d\n", psup->s_inodes_count);
	printf("Inodos libres = %d\n", psup->s_free_inodes_count);
	printf("Bloques particion = %d\n", psup->s_blocks_count);
	printf("Bloques libres = %d\n", psup->s_free_blocks_count);
	printf("Primer bloque de datos = %d\n", psup->s_first_data_block);
}

//PARA EL COMANDO BYTEMAPS
//RECORREMOS ext_bytemaps MOSTRANDO SUS INODOS Y SUS BLOQUES
void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps){
	int i;
	printf("Inodos: ");
	for(i=0;i<24;i++){
		printf("%d ", ext_bytemaps->bmap_inodos[i]);
	}
	printf("\n");
	printf("Bloques [0-25]: ");
	for(i=0;i<25;i++){
		printf("%d ", ext_bytemaps->bmap_bloques[i]);
	}
	printf("\n");
}

//RENAME
//nombreantiguo y nombrenuevo SERAN argumento1 y argumento2
//RECORREMOS TODOS LOS DIRECTORIOS DESCARTANDO LOS QUE SU INODO SEAN 0xFFFF Y COGEMOS EL QUE SU NOMBRE SEA EL MISMO QUE nombreantiguo
//CON STRCPY CAMBIAMOS EL NOMBRE DEL FICHERO POR EL nombrenuevo (argumento2)
int Renombrar(EXT_ENTRADA_DIR *directorio, char *nombreantiguo, char *nombrenuevo){
	int j;
	//solo entrara aqui si el comando es correcto
	for(j=1; j<MAX_FICHEROS;j++){
		if (directorio[j].dir_inodo!=NULL_INODO){
			if(strcmp(nombreantiguo, directorio[j].dir_nfich)==0)
			{
				strcpy(directorio[j].dir_nfich, nombrenuevo);
				break;
			}
		}
	}
	
	return 1;
}

//DIR
//RECORRE TODOS LOS DIRECTORIOS CUYO INODO NO SEA 0xFFFF E IMPRIMIRA SU NOMBRE, TAMAÑO E INODO
//DESPUES CON EL INODO SACAREMOS EL BLOQUE E IMPRIMIREMOS TODOS LOS VALORES DEL ARRAY DE BLOQUES DISTINTOS A 0xFFFF (NULL_BLOQUE)
void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos){
	int j;
	for(j=1; j<MAX_FICHEROS;j++){
		if (directorio[j].dir_inodo!=NULL_INODO){		
			printf("%s\t", directorio[j].dir_nfich);
			printf("tamaño: %d\t", inodos->blq_inodos[directorio[j].dir_inodo].size_fichero);
			printf("inodo: %d\t", directorio[j].dir_inodo);
			printf("bloques: ");
			for(int i=0;i<MAX_NUMS_BLOQUE_INODO;i++){
				if(inodos->blq_inodos[directorio[j].dir_inodo].i_nbloque[i]!=NULL_BLOQUE)
					printf("%d ", inodos->blq_inodos[directorio[j].dir_inodo].i_nbloque[i]);
			}
			printf("\n");
		}
	}
}

//IMPRIMIR
//COGEMOS EL DIRECTORIO CUYO NOMBRE SEA EL MISMO DE argumento1 
//POR CADA BLOQUE DISTINTO DE NULL_BLOQUE IMPRIMIMOS LOS DATOS CORRESPONDIENTES A ESE BLOQUE -4 (esos 4 bloques estan reservados por lo que el bloque 4 será el memdatos[0])
int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre){
	for(int j=1; j<MAX_FICHEROS;j++){
		if (directorio[j].dir_inodo!=NULL_INODO){		
			if(strcmp(nombre,directorio[j].dir_nfich)==0){
				for(int i=0;i<MAX_NUMS_BLOQUE_INODO;i++){
					if(inodos->blq_inodos[directorio[j].dir_inodo].i_nbloque[i]!=NULL_BLOQUE){
						for(int k =0;k<SIZE_BLOQUE;k++)
							printf("%c", memdatos[inodos->blq_inodos[directorio[j].dir_inodo].i_nbloque[i]-4].dato[k]);
					}				
				}
				return 1;
			}
		}
	}
	printf("no ha encontrado el fichero\n");
	return 0;
}

//REMOVE
//ELEGIMOS EL QUE HA ELEGIDO EL USUARIO
//PRIMERO BORRAMOS DEL BYTEMAP_INODOS SU POSICION A 0
//DESPUES RECORREMOS LOS BLOQUES Y PONEMOS A 0 SUS CORRESPONDIENTES POSICIONES Y DESPUES PONEMOS ESE MISMO NUMERO A NULL_BLOQUE
//DESPUES ESTABLECEMOS EL INODO A NULL_INODO Y BORRAMOS EL NOMBRE DEL DIRECTORIO ESTABLECIENDOLO A "\0"
int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos,EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock,char *nombre){ //FILE *fich
	int j;	
	for(int j=1; j<MAX_FICHEROS;j++){
		if (directorio[j].dir_inodo!=NULL_INODO){	
			if(strcmp(nombre,directorio[j].dir_nfich)==0){ //elegimos el que ha elegido el usuario
				ext_bytemaps->bmap_inodos[directorio[j].dir_inodo]=0; //bytemaps de inodos a 0
				for(int i=0;i<MAX_NUMS_BLOQUE_INODO;i++){
					if(inodos->blq_inodos[directorio[j].dir_inodo].i_nbloque[i]!=NULL_BLOQUE)
						ext_bytemaps->bmap_bloques[inodos->blq_inodos[directorio[j].dir_inodo].i_nbloque[i]]=0; //bytemaps de bloques a 0
						inodos->blq_inodos[directorio[j].dir_inodo].i_nbloque[i]=NULL_BLOQUE; //nbloque a FFFF
				}
				directorio[j].dir_inodo=NULL_INODO; //inodo a FFFF
				strcpy(directorio[j].dir_nfich, "\0"); //borrar nombre
			}
		}
	}
	return 1;
}

//RESPETAMOS LA ESTRUCTURA POR DEFECTO DE LA PRACTICA
int main()
{
	 char *comando[LONGITUD_COMANDO];
	 char *orden[LONGITUD_COMANDO];
	 char *argumento1[LONGITUD_COMANDO];
	 char *argumento2[LONGITUD_COMANDO];
	 
	 int i,j;
	 unsigned long int m;
     EXT_SIMPLE_SUPERBLOCK ext_superblock;
     EXT_BYTE_MAPS ext_bytemaps;
     EXT_BLQ_INODOS ext_blq_inodos;
     EXT_ENTRADA_DIR directorio[MAX_FICHEROS];
     EXT_DATOS memdatos[MAX_BLOQUES_DATOS];
     EXT_DATOS datosfich[MAX_BLOQUES_PARTICION];
     int entradadir;
     int grabardatos;
     FILE *fent;
     
     // Lectura del fichero completo de una sola vez
     //...

	 int compruebanombre=0;
     fent = fopen("particion.bin","r+b");
     fread(&datosfich, SIZE_BLOQUE, MAX_BLOQUES_PARTICION, fent);    
     
     memcpy(&ext_superblock,(EXT_SIMPLE_SUPERBLOCK *)&datosfich[0], SIZE_BLOQUE);
     memcpy(&directorio,(EXT_ENTRADA_DIR *)&datosfich[3], SIZE_BLOQUE);
     memcpy(&ext_bytemaps,(EXT_BLQ_INODOS *)&datosfich[1], SIZE_BLOQUE);
     memcpy(&ext_blq_inodos,(EXT_BLQ_INODOS *)&datosfich[2], SIZE_BLOQUE);
     memcpy(&memdatos,(EXT_DATOS *)&datosfich[4],MAX_BLOQUES_DATOS*SIZE_BLOQUE);
     
     // Bucle de tratamiento de comandos
	for (;;){
		do {
			printf ("\n>> ");
			fflush(stdin);
			fgets(comando, LONGITUD_COMANDO, stdin);
		} while (ComprobarComando(comando, orden, argumento1, argumento2) !=0); //LLAMAMOS A COMPROBARCOMANDO QUE SI LA CADENA ES VALIDA DEVOLVERÁ 0 Y SALDRA DEL BUCLE
		
	    if (strcmp(orden,"dir")==0) {
            Directorio(&directorio,&ext_blq_inodos); //SI HA SELECCIONADO dir SE LLAMARÁ A LA FUNCION Directorio
            continue;
        }
		if(strcmp(orden, "info")==0){
			LeeSuperBloque(&ext_superblock); //CUANDO PONE info SE LLAMA A LA FUNCION LeeSuperBloque
			continue;
		}  
		if(strcmp(orden,"bytemaps")==0){ //CUANDO PONE bytemaps LLAMA A LA FUNCION Printbytemaps
			Printbytemaps(&ext_bytemaps);
			continue;
		}
		  
		if(strcmp(orden,"rename")==0){
			for(int j=1; j<MAX_FICHEROS;j++){
				if (directorio[j].dir_inodo!=NULL_INODO){	 //comprobamos que argumento1 es el nombre de un fichero que ya existe
					if(strcmp(argumento1, directorio[j].dir_nfich)==0)
					{
						compruebanombre=1;
					}
				}
			}
			if(compruebanombre==0){
				printf("ERROR: fichero %s no encontrado\n", argumento1);
				continue;
			}
			compruebanombre=0;
			
			for(int j=1; j<MAX_FICHEROS;j++){
				if (directorio[j].dir_inodo!=NULL_INODO){	 //comprobamos que argumento2 no es el nombre de un fichero que ya existe
					if(strcmp(argumento2, directorio[j].dir_nfich)==0)
					{
						compruebanombre=1;
					}
				}
			}
			if(compruebanombre!=0){
				printf("ERROR: el fichero %s ya existe\n", argumento2);
				continue;
			}
			Renombrar(&directorio, argumento1, argumento2); //SI LOS DOS ARGUMENTOS SON VALIDOS LLAMARA A RENOMBRAR
			continue;
		}
		
		if(strcmp(orden,"imprimir")==0){
			Imprimir(&directorio, &ext_blq_inodos, &memdatos, &argumento1); //LLAMA A IMPRIMIR
			continue;
		}
		
		if(strcmp(orden,"remove")==0){
			for(int j=1; j<MAX_FICHEROS;j++){ //COMPRUEBA QUE EL FICHERO EXISTE
				if (directorio[j].dir_inodo!=NULL_INODO){					
					if(strcmp(argumento1, directorio[j].dir_nfich)==0)
					{
						compruebanombre=1;
					}
				}
			}
			if(compruebanombre==0){
				printf("ERROR: fichero %s no encontrado\n", argumento1);
				continue;
			}
			Borrar(&directorio, &ext_blq_inodos, &ext_bytemaps, &ext_superblock, &argumento1); //FILE *fich LLAMA A Borrar
			
			continue;
		}
		
		if(strcmp(orden,"copy")==0){
			printf("No nos ha dado tiempo a hacer esta funcion, disculpe las molestias\n");
			continue;
		}
        // añadir comentarios...
        // Escritura de metadatos en comandos rename, remove, copy     
         // Grabarinodosydirectorio(&directorio,&ext_blq_inodos,fent);
         // GrabarByteMaps(&ext_bytemaps,fent);
         // GrabarSuperBloque(&ext_superblock,fent);
         // if (grabardatos)
           // GrabarDatos(&memdatos,fent);
         // grabardatos = 0;
        // Si el comando es salir se habrán escrito todos los metadatos
        // faltan los datos y cerrar
          if (strcmp(orden,"salir")==0){
            // GrabarDatos(&memdatos,fent);
			printf("has elegido salir");
            fclose(fent);
            return 0;
          }
    } 
}
	
