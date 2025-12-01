#include <iostream>
#include <unistd.h>
#include <cstdio>
#include <string>
#include <cstdlib>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <ctime>
using namespace std;
#include <sys/sem.h>
#include <cstring>

#define QUESTIONS 5
#define EXAMS 20
#define SEMAPHORE_RUBRIC 0
#define SEMAPHORE_EXAM 1
#define SEMAPHORE_QUESTIONS 2

const char *exam_files[EXAMS]={"exam_files/exam1.txt","exam_files/exam2.txt","exam_files/exam3.txt","exam_files/exam4.txt","exam_files/exam5.txt","exam_files/exam6.txt","exam_files/exam7.txt",
"exam_files/exam8.txt","exam_files/exam9.txt","exam_files/exam10.txt","exam_files/exam11.txt","exam_files/exam12.txt","exam_files/exam13.txt","exam_files/exam14.txt","exam_files/exam15.txt",
"exam_files/exam16.txt","exam_files/exam17.txt","exam_files/exam18.txt","exam_files/exam19.txt","exam_files/exam20.txt"};

/**
union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

*if you are on macOS, keep this union commented out because it gives an error,
* however if you're on Windows, uncomment it so it gets used.
*/

typedef struct exam_info
{
    int student_id;
    int curr_exam_index;
    char rubric[QUESTIONS];
    int questions_marked[QUESTIONS];
    bool end_marking;
}exam_info_t;

void delay(float num1, float num2)
{
    float random=(float)rand()/(float)RAND_MAX;
    float sec=num1+random*(num2-num1);
    useconds_t usec=(useconds_t)(sec*1000000.0);
    usleep(usec);
}

void sem_wait(int semid, int semnum)
{
    struct sembuf operation;
    operation.sem_num=semnum;
    operation.sem_op=-1;
    operation.sem_flg=0;
    semop(semid, &operation, 1);
}

void sem_signal(int semid, int semnum)
{
    struct sembuf operation;
    operation.sem_num=semnum;
    operation.sem_op=1;
    operation.sem_flg=0;
    semop(semid, &operation, 1);
}

int rubric_loading(exam_info_t *exam)
{
    FILE *file=fopen("rubric.txt","r");
    if (!file)
    {
        printf("File cannot be opened or does not exist.\n");
        return -1;
    }
    char line[10];
    for (int i=0; i<QUESTIONS; i++)
    {
        if (!fgets(line, sizeof(line),file))
        {
            printf("Rubric missing a letter.\n");
            fclose(file);
            return -1;
        }
        char *comma=strchr(line,',');
        if (!comma)
        {
            fclose(file);
            return -1;
        }
        comma++;
        while (*comma==' ')
        {
            comma++;
        }
        exam->rubric[i]=*comma;
    }
    fclose(file);
    return 0;
}

int rubric_updating(exam_info_t *exam)
{
    FILE *file=fopen("rubric.txt","w");
    if (!file) 
    {
        printf("Cannor open rubric.\n");
        return -1;
    }
    for (int i=0; i<QUESTIONS; i++)
    {
        fprintf(file, "%d, %c\n", i+1, exam->rubric[i]);
    }
    fclose(file);
    return 0;
}

int exam_loading(exam_info_t *exam, const char *filename)
{
    FILE *file=fopen(filename,"r");
    if (!file)
    {
        printf("File cannot be opened or does not exist.\n");
        return -1;
    }
    char line[6];
    if (!fgets(line, sizeof(line), file))
    {
        printf("Exam file is empty.\n");
        fclose(file);
        return -1;
    }
    exam->student_id=stoi(line);
    for (int i=0; i<QUESTIONS; i++)
    {
        exam->questions_marked[i]=0;
    }
    fclose(file);
    return 0;
}

bool marked_all_questions(exam_info_t *info)
{
    for (int i=0; i<QUESTIONS; i++)
    {
        if (info->questions_marked[i]==0)
        {
            return false;
        }
    }
    return true;
}

void review_rubric(exam_info_t *info, int ta, int semid)
{
    printf("TA #%d reviewing rubric for student %d.\n", ta, info->student_id);
    for (int i=0; i<QUESTIONS; i++)
    {
        delay(0.5, 1);
        int change_rubric=rand()%5;
        if (change_rubric==0)
        {
            sem_wait(semid, SEMAPHORE_RUBRIC);
            char old_rubric_letter=info->rubric[i];
            info->rubric[i]=old_rubric_letter+1;
            printf("TA #%d corrected the rubric for question %d: from %c to %c\n", ta, i+1, old_rubric_letter, info->rubric[i]);
            rubric_updating(info);
            sem_signal(semid, SEMAPHORE_RUBRIC);
        }
    }
}

void ta_processing(exam_info_t *info, int ta, int semid)
{
    srand(time(NULL)^(getpid()<<16));
    int last_exam_index=-1;
    printf("TA #%d started marking\n", ta);
    while (!info->end_marking)
    {
        if (info->student_id==-1)
        {
            usleep(100000);
            continue;
        }
        int sid=info->student_id;
        int curr_exam=info->curr_exam_index;
        if (sid==9999)
        {
            printf("TA #%d is on exam with student ID 9999, all TA's must stop.\n", ta);
            info->end_marking=true;
            break;
        }
        if (curr_exam!=last_exam_index)
        {
            printf("TA #%d loaded new exam to mark.\n", ta);
            review_rubric(info, ta, semid);
            last_exam_index=curr_exam;
        }
        int one_question=0;
        sem_wait(semid, SEMAPHORE_QUESTIONS);
        for (int i=0; i<QUESTIONS; i++)
        {
            if (info->questions_marked[i]==0)
            {
                info->questions_marked[i]=1;
                printf("TA #%d has marked the question %d for student %d.\n", ta, i+1, sid);
                delay(1, 2);
                one_question=1;
                break;
            }
        }
        sem_signal(semid, SEMAPHORE_QUESTIONS);
        if (!one_question)
        {
            sem_wait(semid, SEMAPHORE_EXAM);
            if (marked_all_questions(info))
            {
                int next=info->curr_exam_index+1;
                if (next>=EXAMS)
                {
                    printf("TA #%d has no more exams to mark.\n", ta);
                    info->end_marking=true;
                }
                else
                {
                    printf("TA #%d finished marking exam, now loading next exam.\n", ta);
                    if (exam_loading(info, exam_files[next])==0)
                    {
                        info->curr_exam_index=next;
                        printf("TA #%d loaded exam %s for student %d.\n", ta, exam_files[next], info->student_id);
                        if (info->student_id==9999)
                        {
                            printf("TA #%d is on exam with student ID 9999, all TA's must stop.\n", ta);
                            info->end_marking=true;
                        }
                    }
                    else
                    {
                        printf("TA #%d couldnt load the exam.\n", ta);
                        info->end_marking=true;
                    }
                    }
                }
            sem_signal(semid, SEMAPHORE_EXAM);
            usleep(100000);
        }
    }
    printf("TA #%d exiting.\n", ta);
}

int main(int argc, char *argv[])
{
    if (argc<2)
    {
        printf("Need to use ./a3_p2a <number_of_TAs>.\n");
        return 1;
    }
    int numTAs=stoi(argv[1]);
    if (numTAs<2)
    {
        printf("There needs to be at least 2 TAs.\n");
        return 1;
    }
    int shmid=shmget(IPC_PRIVATE, sizeof(exam_info_t), IPC_CREAT|0666);
    if (shmid<0)
    {
        return 1;
    }
    exam_info_t *info=(exam_info_t*)shmat(shmid, NULL, 0);
    info->student_id=-1;
    info->end_marking=false;
    for (int i=0; i<QUESTIONS; i++)
    {
        info->questions_marked[i]=0;
    }
    if (rubric_loading(info)!=0)
    {
        shmdt(info);
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }
    if (exam_loading(info, exam_files[0])!=0)
    {
        shmdt(info);
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }
    info->curr_exam_index=0;
    int semid=semget(IPC_PRIVATE, 3, IPC_CREAT|0666);
    if (semid<0)
    {
        shmdt(info);
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }
    unsigned short num[3]={1,1,1};
    union semun arg;
    arg.array=num;
    if (semctl(semid, 0, SETALL, arg)<0)
    {
        shmdt(info);
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID);
        return 1;
    }
    for (int i=0; i<numTAs; i++)
    {
        pid_t pid=fork();
        if (pid<0)
        {
            printf("forking failed.\n");
        }
        else if (pid==0)
        {
            ta_processing(info, i+1, semid);
            shmdt(info);
            _exit(0);
        }
    }
    for (int i=0; i<numTAs; i++)
    {
        wait(NULL);
    }
    printf("All TAs have completed marking.\n");
    shmdt(info);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    return 0;
}