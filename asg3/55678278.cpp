//CS3103 Programming Assignment 3
//Created by 55678278

#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <iomanip>
#include <cmath>
using namespace std;

#define MAXIMUM_FRAME_NUM 10
#define MAXIMUM_FRAME_CAP 5

class Node{
public:
    void* data;
    Node* next;
    Node(void*);
};

Node::Node(void* arg){
    this->data = arg;
    this->next = NULL;
}

class Queue{
public:
    Queue();
    ~Queue();
    bool isEmpty();
    bool isFull();
    bool hasOne();
    void push_back(void*);
    void* top_front();
    void* pop_front();
private:
    Node* front;
    Node* rear;
    int size;
};

Queue::Queue(){
    this->front = NULL;
    this->rear = NULL;
    this->size = 0;
}

Queue::~Queue(){
    Node* cur = front;
    
    while (cur) {
        Node* nex = cur->next;
        delete cur;
        cur = nex;
    }
    
    delete cur;
    delete this->front;
    delete this->rear;
}

bool Queue::isEmpty(){return front==NULL;}

bool Queue::isFull(){return size>=MAXIMUM_FRAME_CAP;}

bool Queue::hasOne(){return size==1;}

void Queue::push_back(void* arg){
    if (isEmpty()) {
        front = new Node(arg);
        rear = front;
    }
    else{
        rear->next = new Node(arg);
        rear = rear->next;
    }

    size++;
}

void* Queue::top_front(){return front->data;}

void* Queue::pop_front(){
    Node* n = front;
    front = front->next;
    
    size--;
    
    return n->data;
}

Queue* cache;
double* temp_frame_recorder;
int frame_length = 4*2;

static pthread_mutex_t cacheFront = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t cacheRear = PTHREAD_MUTEX_INITIALIZER;

sem_t generateFrame;//estimator() to camera()
sem_t transformFrame;//camera() to transformer()
sem_t transformFrameNext;//estimator() to transformer() of next round
sem_t computeMSE;//transformer() to estimator()

double* generate_frame_vector(int l);

void* camera(void *arg){
    int interval = *(int*)arg;
    bool cacheFrontLocked = false;
    double* frame_vector;
    
    while(1){
        sem_wait(&generateFrame);
        
        frame_vector = generate_frame_vector(frame_length);
        
        if(!frame_vector)
            break;
        else{
            if(cache->hasOne()){//only 1 frame in cache, should lock front and block estimator
                pthread_mutex_lock(&cacheFront);
                cacheFrontLocked = true;
            }
            
            pthread_mutex_lock(&cacheRear);
            cache->push_back(frame_vector);
            sleep(interval);
            pthread_mutex_unlock(&cacheRear);
            
            if(cacheFrontLocked){//unlock front if locked
                pthread_mutex_unlock(&cacheFront);
                cacheFrontLocked = false;
            }
        }
        
        sem_post(&transformFrame);
    }
    
    pthread_exit(NULL);
}

void* transformer(void *arg){
    static int processedFrames = 0;//count for termination condition
    
    double* frame_vector;
    int i;
    
    temp_frame_recorder = (double*)malloc(frame_length * sizeof(double));
    
    while(1){
        sem_wait(&transformFrame);
	sem_wait(&transformFrameNext);//only start after previous round finished
        
        frame_vector = (double*)cache->top_front();//reading frame only, no mutex required
        
        for(i=0;i<frame_length;++i)
            temp_frame_recorder[i] = frame_vector[i];
        
        for(i=0;i<frame_length;++i)
            temp_frame_recorder[i] = round(10*temp_frame_recorder[i]);
        
        for(i=0;i<frame_length;++i)
            temp_frame_recorder[i] = temp_frame_recorder[i]/10;
        
        sleep(3);
        
        sem_post(&computeMSE);
        
        if(++processedFrames >= MAXIMUM_FRAME_NUM)
            break;
    }
    
    pthread_exit(NULL);
}

void* estimator(void *arg){
    static int processedFrames = 0;//count for termination condition
    
    double* frame_vector;
    int i;
    double MSE;
    
    while(1){
        MSE = 0;
        sem_wait(&computeMSE);
        
        pthread_mutex_lock(&cacheFront);
        frame_vector = (double*)cache->top_front();
        
        for(i=0;i<frame_length;++i)
            MSE += pow((frame_vector[i]-temp_frame_recorder[i]), 2.0);
        
        MSE /= frame_length;
        
        cout<<setprecision(6)<<fixed<<"mse = "<<MSE<<endl;
        
        cache->pop_front();
        pthread_mutex_unlock(&cacheFront);
        
        sem_post(&generateFrame);
	sem_post(&transformFrameNext);//current frame deleted and finished, notify transformer() of next round to start
        
        if(++processedFrames >= MAXIMUM_FRAME_NUM)
            break;
    }
    
    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    int cam, tran, est;
    pthread_t pthCam, pthTran, pthEst;
    cache = new Queue();
    
    if(!isdigit(*argv[1])){
        cout<<"invalid input...\n";
        exit(EXIT_FAILURE);
    }
    
    int interval = atoi(argv[1]);
    
    sem_init(&generateFrame, 0, 5);//max frame cap = 5
    sem_init(&transformFrame, 0, 0);
    sem_init(&transformFrameNext, 0, 1);//transformer in 1st round does not need to wait for previous round
    sem_init(&computeMSE, 0, 0);
    
    cam = pthread_create(&pthCam, NULL, camera, &interval);
    tran = pthread_create(&pthTran, NULL, transformer, NULL);
    est = pthread_create(&pthEst, NULL, estimator, NULL);
    
    if(cam||tran||est){
        cout<<"thread creation error...\n";
        exit(EXIT_FAILURE);
    }
    
    cam = pthread_join(pthCam, NULL);
    tran = pthread_join(pthTran, NULL);
    est = pthread_join(pthEst, NULL);
    
    if(cam||tran||est){
        cout<<"thread joining error...\n";
        exit(EXIT_FAILURE);
    }
    
    delete cache;
    delete temp_frame_recorder;

    sem_destroy(&generateFrame);
    sem_destroy(&transformFrame);
    sem_destroy(&transformFrameNext);
    sem_destroy(&computeMSE);

    pthread_exit(NULL);
    //return 0;
}
