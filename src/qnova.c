#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define VERSION "QBIT_NOVA_C_V0.2_QUANTUM"
#define MAX_VARS 64
#define MAX_QBITS 32

/* ── Variables ── */
typedef struct { char name[64]; double value; } Var;
Var vars[MAX_VARS];
int var_count = 0;

double get_var(char *n) {
    for (int i=0;i<var_count;i++)
        if(strcmp(vars[i].name,n)==0) return vars[i].value;
    return 0;
}
void set_var(char *n, double v) {
    for (int i=0;i<var_count;i++)
        if(strcmp(vars[i].name,n)==0){vars[i].value=v;return;}
    strcpy(vars[var_count].name,n);
    vars[var_count++].value=v;
}

/* ── Qbits ── */
typedef struct {
    char name[64];
    double a; /* |0> amplitude */
    double b; /* |1> amplitude */
    int measured;
    int result;
} Qbit;
Qbit qbits[MAX_QBITS];
int qbit_count = 0;

Qbit* get_qbit(char *n) {
    for(int i=0;i<qbit_count;i++)
        if(strcmp(qbits[i].name,n)==0) return &qbits[i];
    return NULL;
}

void print_qbit(Qbit *q) {
    if(q->measured)
        printf("|%d>\n", q->result);
    else
        printf("|0>=%.3f |1>=%.3f\n", q->a, q->b);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    printf("%s\n", VERSION);
    if(argc<2){printf("Usage: qnova <file.qn>\n");return 0;}

    FILE *f = fopen(argv[1],"r");
    if(!f){printf("Error: file not found\n");return 1;}

    char line[512];
    int skip=0;

    while(fgets(line,sizeof(line),f)) {
        line[strcspn(line,"\n")]=0;
        char *t=line;
        while(*t==' ')t++;

        if(t[0]=='@') continue;
        if(t[0]=='}'){skip=0;continue;}
        if(skip) continue;

        /* say */
        if(strncmp(t,"say ",4)==0) {
            char *msg=t+4;
            Qbit *q=get_qbit(msg);
            if(q){print_qbit(q);continue;}
            if(msg[0]=='"'){msg++;msg[strlen(msg)-1]=0;printf("%s\n",msg);}
            else printf("%g\n",get_var(msg));
        }

        /* let */
        else if(strncmp(t,"let ",4)==0) {
            char n[64]; double v;
            sscanf(t+4,"%s = %lf",n,&v);
            set_var(n,v);
        }

        /* check */
        else if(strncmp(t,"check ",6)==0) {
            char vn[64],op[4]; double rhs;
            sscanf(t+6,"%s %s %lf",vn,op,&rhs);
            double lhs=get_var(vn);
            int r=0;
            if(strcmp(op,">")==0)r=lhs>rhs;
            if(strcmp(op,"<")==0)r=lhs<rhs;
            if(strcmp(op,"==")==0)r=lhs==rhs;
            if(!r)skip=1;
        }

        /* qbit q = |0> */
        else if(strncmp(t,"qbit ",5)==0) {
            char n[64],state[8];
            sscanf(t+5,"%s = %s",n,state);
            Qbit q; memset(&q,0,sizeof(q));
            strcpy(q.name,n);
            if(strcmp(state,"|0>")==0){q.a=1.0;q.b=0.0;}
            else{q.a=0.0;q.b=1.0;}
            q.measured=0;
            qbits[qbit_count++]=q;
        }

        /* hadamard q */
        else if(strncmp(t,"hadamard ",9)==0) {
            char n[64]; sscanf(t+9,"%s",n);
            Qbit *q=get_qbit(n);
            if(q){
                double a=q->a,b=q->b;
                q->a=(a+b)/sqrt(2);
                q->b=(a-b)/sqrt(2);
                q->measured=0;
            }
        }

        /* measure q */
        else if(strncmp(t,"measure ",8)==0) {
            char n[64]; sscanf(t+8,"%s",n);
            Qbit *q=get_qbit(n);
            if(q){
                double prob0=q->a*q->a;
                double r=(double)rand()/RAND_MAX;
                q->result=(r>prob0)?1:0;
                q->measured=1;
                if(q->result==0){q->a=1;q->b=0;}
                else{q->a=0;q->b=1;}
            }
        }
    }
    fclose(f);
    return 0;
}
