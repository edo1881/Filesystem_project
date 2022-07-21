#include "fat32.h"
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>  
#include "linked_list.h"
#include "linenoise.h"

#define NUMBLOCKS 100
#define NUMCMD 11
#define LENCMD 128
#define LENPATH 128
#define LINE 1024


char* cmd[NUMCMD]= {
    "createfile",
    "ls","open","close",
    "write","read","seek",
    "cd","mkdir","rm","exit"
};

char** ris=NULL;
int num_cwd_entries=0;

void completion(const char *buf, linenoiseCompletions *lc) {
    //printf("buf è :%s e lenbuf è:%ld\n",buf,strlen(buf));
    assert(ris!=NULL && ris[0]!=NULL && "errorr");
    //printf("il primo è %c",ris[0][0]);
    //printf("sono in callback");
    char* copy=(char*)malloc(3);
    strncpy(copy,buf,2);
    copy[2]='\0';
    //printf("copy: %s num: %d\n",copy,num_cwd_entries);
    //printf("sto prima");
    //fflush(stdout);
    if(strcmp(copy,"cd")) {
        printf("Esco...\n");
        return;
    }
    //printf("sto dopo");
    //fflush(stdout);
    for(int i=0;i<num_cwd_entries;i++) {
         if(ris[i] && ris[i][0]==buf[strlen(buf)-1]){
            //printf("sto in if e strlen vale:%ld",strlen(ris[i]));
            //fflush(stdout);

            char* to_add=(char*)malloc(20);
            strcpy(to_add,"cd ");
            strcat(to_add,ris[i]);
            //printf("sto per invocare linenoise");
            //fflush(stdout);
            linenoiseAddCompletion(lc,to_add);
            free(to_add);
            //printf("fatto linenoiseadd");
            //fflush(stdout);
         }
     }
    //printf("esco dal for");
    //fflush(stdout);
}

void ris_add(char* name) {
    if(name) {
        //printf("I will add: %s strlen:%ld\n",name,strlen(name));
        char** p=ris;
        int i =0;
       // assert(ris && p && "error");
        while(p && *p) {
            //printf("%s\n",*p);
            i++;
            p++;
        }
        printf("i:%d\n",i);
        //if(!p) realloc(ris,
        int len=strlen(name);
        ris[i]=(char*)malloc(len+1);
        memcpy(ris[i],name,len);
        ris[i][len]='\0';
        //printf("Ho aggiunto %s in ris\n",ris[i]);
    }
}
void ris_rm(char* name) {
    char** p=ris;
    while(*p){
        if(!strcmp(*p,name)) {
            //printf("HO rimosso %s da ris\n",*p);
            free(*p);
            memset(p,0,sizeof(char*));
            break;
        }
        p++;
    }
}
int main(int argc, char** argv) {
    if(argc<2) {
        printf("Inserire il nome del disco\n");
        return 0;
    }
    int ret;


    //**********************READ THE DISK*******************************************
    
    char filename[64];
    strcpy(filename,argv[1]);
    DiskDriver* disk=(DiskDriver*)malloc(sizeof(DiskDriver));
    fat32* fs=(fat32*)malloc(sizeof(fat32));
    DirectoryHandle* root;
    ris=filename_alloc();
    if((ret=open(filename,O_RDWR | O_SYNC,0666))==-1){
        DiskDriver_init(disk,filename,NUMBLOCKS);
        root=fat32_init(fs,disk);
    }
    else{
        root= (DirectoryHandle*)malloc(sizeof(DirectoryHandle));
        disk->fd=ret;
        disk->header=mmap (0, sizeof(DiskHeader), PROT_READ | PROT_WRITE, MAP_SHARED, disk->fd, 0);
        disk->fat =mmap(0,100*sizeof(int),PROT_READ | PROT_WRITE,MAP_SHARED,disk->fd,sysconf(_SC_PAGE_SIZE));
        fs->disk=disk;
        fs->cwd=(FirstDirectoryBlock*)malloc(BLOCK_SIZE);
        ret=DiskDriver_readBlock(disk,fs->cwd,0);
        assert(ret && "Errore lettura disco");
        root->directory=NULL;
        root->dcb=fs->cwd;
        root->f=fs;
        num_cwd_entries=root->dcb->num_entries;
        fat32_listDir(ris,root);
    }
    /**********************************************************************/



    
    printf("Welcome to our fancy FileSystem, here's a list of what u can do:\n");
    for(int i=0;i<NUMCMD;i++) {
        printf("%s\t",cmd[i]);
        if(i!=0 && i%2==0) printf("\n");
    }
    char* path=malloc(LENPATH);
    strcpy(path,root->dcb->fcb.name);


    ListHead* head=(ListHead*)malloc(sizeof(ListHead));
    List_init(head);
    FileHandle* fh;

    char* user_cmd;
    linenoiseSetCompletionCallback(completion);
    /*linenoiseCompletions* lc =(linenoiseCompletions*)malloc(sizeof(linenoiseCompletions));
    lc->len=1;
    lc->cvec=(char**)malloc(sizeof(char*));*/
    while(1) {
        //printf("i'm in %s\n",fs->cwd->fcb.name);
        //printf("%s : ",path);
        num_cwd_entries=fs->cwd->num_entries;
        user_cmd= linenoise(path);
        /*assert(ris&& "ris not allocated");
        for(int i=0;i<root->dcb->num_entries;i++){
            assert(ris[i]&& "ris not allocated");
            printf("i: %d value:%s at %p\n",i,ris[i],ris[i]);

        }*/
        //printf("echo: %s\n",line);
        //fflush(stdin);
        //fgets(user_cmd, LENCMD, stdin);
        //remove new line
        if ((strlen(user_cmd) > 0) && (user_cmd[strlen (user_cmd) - 1] == '\n'))
            user_cmd[strlen (user_cmd) - 1] = '\0';
        char* CMD;
        char* ARG;

        CMD=strtok(user_cmd," ");
        ARG=strtok(NULL," ");
        //printf("CMD: %s\n",CMD);
   
        if(CMD){
            if(!strcmp(CMD,"exit"))
            {
                break;
            }
            else if (!strcmp(CMD,"ls"))
            {
                if(root->dcb->num_entries>0){
                    char** names=filename_alloc();
                    int bitmap=fat32_listDir(names,root);
                    int mask;
                    for(int i=0;i<root->dcb->num_entries;i++){
                        mask=1;
                        (bitmap&(mask<<i))? printf("\x1b[31m%s\33[0m\n",names[i]) : printf("%s\n",names[i]);
                    }
                    filename_dealloc(names);
                }
            }
            else if (!strcmp(CMD,"cd")){
                if(!ARG) {
                    printf("usage: cd <nomefile>\n");
                    continue;
                }
                if(!strcmp(ARG,"..") && strcmp(root->dcb->fcb.name,"/\0")){
                    int len2=strlen(root->dcb->fcb.name)+1;
                    int len1=strlen(path);
                    int newlen=len1-len2;
                    char* s=(char*)calloc(1,LENPATH);
                    char*temp=path;
                    strncpy(s,path,newlen);   
                    s[strlen(s)]='\0';                     
                    path=s;
                    free(temp);
                    ret=fat32_changeDir(root,ARG);
                    if(ret==-1) {
                        printf("something went wrong\n");
                    }
                    
                        
                }
                else if(!strcmp(ARG,"..") && !strcmp(root->dcb->fcb.name,"/\0")){

                }
                else{
                    ret=fat32_changeDir(root,ARG);
                    if(ret==-1) {
                        printf("no such file or directory: %s\n",ARG);
                    }
                    else{
                        strcat(path,root->dcb->fcb.name);                        
                        strcat(path,"/\0");
                        

                    }
                        
                    }
                
            }
            else if (!strcmp(CMD,"mkdir"))
            {
                if(!ARG) {
                    printf("usage: mkdir <nomefile>\n");
                    continue;
                }    
                fat32_mkDir(root,ARG);
                ris_add(ARG);
            }
            else if (!strcmp(CMD,"createfile"))
            {
                if(!ARG) {
                    printf("usage: createfile <nomefile>\n");
                    continue;
                }
                fh=fat32_createFile(root,ARG);
                if(fh) {
                    List_insert(head,NULL,(ListItem*)fh);
                    ris_add(ARG);
                    //List_print(head);
                }
            }
            else if(!strcmp(CMD,"open")) {
                if(!ARG) {
                    printf("usage: open <nomefile>\n");
                    continue;
                }
                fh=fat32_openFile(root,ARG);
                if(fh) {
                    List_insert(head,NULL,(ListItem*)fh);
                    
                    //List_print(head);
                }
            }
            else if(!strcmp(CMD,"close")) {
                if(!ARG) {
                    printf("usage: close <nomefile>\n");
                    continue;
                }
                ListItem* item=List_find(head,ARG);
                List_detach(head,item);
                //List_print(head);
                fat32_close((FileHandle*)item);
            }

            else if(!strcmp(CMD,"write")) {
                if(!ARG) {
                    printf("usage: write <nomefile>\n");
                    continue;
                }
                if((fh = (FileHandle*)List_find(head,ARG))==NULL) {
                    printf("File non aperto o non creato\n");
                    continue;
                }
                printf("Inserire il text (0<=t<=%d)\n",LINE);
                char buff[LINE];
                fflush(stdin);
                fgets(buff,LINE,stdin);
              
                buff[strlen (buff) - 1] = '\0';
                ret=fat32_write(fh,buff,strlen(buff));
                if(ret!=strlen(buff)) {
                    printf("Scrittura parziale del messaggio\n");
                }
                else printf("WRITE SUCCESS bytes_written: %d\n",ret);
            }
            else if(!strcmp(CMD,"seek")) {
                if(!ARG) {
                    printf("usage: seek <nomefile>\n");
                    continue;
                } 
                if((fh = (FileHandle*)List_find(head,ARG))==NULL) {
                    //printf("File handle at :%p\n",fh);
                    printf("File non aperto o non creato\n");
                    continue;
                }
                printf("Inserire la posizione (0<=p<=%d)\n",fh->ffb->fcb.size);
                char p[LINE];
                fflush(stdin);
                fgets(p,LINE,stdin);
                int pos=atoi(p);
                ret=fat32_seek(fh,pos);
            } 
            else if(!strcmp(CMD,"read")) {
                if(!ARG) {
                    printf("usage: read <nomefile>\n");
                    continue;
                } 
                if((fh = (FileHandle*)List_find(head,ARG))==NULL) {
                    //printf("File handle at :%p\n",fh);
                    printf("File non aperto o non creato\n");
                    continue;
                }
                printf("Inserire i bytes da leggere (0<=b<=%d):",fh->ffb->fcb.size);
                char p[LINE];
                fflush(stdin);
                fgets(p,LINE,stdin);
                int size=atoi(p);
                char buff[size];
                ret=fat32_read(fh,buff,size);
                buff[ret]='\0';
                printf("%s",buff);
                printf("\n");
                printf("bytes_read: %d\n",ret);
            }
            else if(!strcmp(CMD,"rm")) { 
                if(!ARG) {
                    printf("usage: rm <nomefile>\n");
                    continue;
                } 
                while((fh = (FileHandle*)List_find(head,ARG))!=0){
                    List_detach(head,(ListItem*)fh);
                    fat32_close(fh);
                }
                ret=fat32_remove(root,ARG);
                ris_rm(ARG);
            }
            else{
                if(strlen(CMD)>0)
                    printf("command not found: %s\n",CMD);
            }
            
        }
        linenoiseFree(user_cmd);
        //List_print(head);     
    }



 
    //cleanup
    //destroy della lista dei filehandle che deve deallocare sia fh che ffb dentro ogni fh
    List_destroy(head);
    free(disk);
    free(fs->cwd);
    free(fs);
    free(root);
    free(path);
    //filename_dealloc(ris);
}