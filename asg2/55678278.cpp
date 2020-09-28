//CS3103 Programming Assignment 2
//Created by 55678278

#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include <iomanip>

using namespace std;

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

void* Queue::top_front(){
    return front->data;
}

void* Queue::pop_front(){
    Node* n = front;
    front = front->next;
    
    size--;
    
    return n->data;
}

Queue* cache;
#define MAXIMUM_FRAME_NUM 10
static pthread_mutex_t cacheFront = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t cacheRear = PTHREAD_MUTEX_INITIALIZER;

double* generate_frame_vector(int l);

void* camera(void *arg){
    int interval = *(int*)arg;
    bool cacheFrontLocked = false;
    
    while(1){
        sleep(interval);
        
        if(cache->isFull())
            continue;
        else{
            double* frame_vector = generate_frame_vector(4*2);
            
            if(!frame_vector)
                break;
            else{
                if(cache->hasOne()){//if only 1 frame in cache, should lock front and block quantizer
                    pthread_mutex_lock(&cacheFront);
                    cacheFrontLocked = true;
                }
                
                pthread_mutex_lock(&cacheRear);
                cache->push_back(frame_vector);
                pthread_mutex_unlock(&cacheRear);
                
                if(cacheFrontLocked){//unlock front if locked
                    pthread_mutex_unlock(&cacheFront);
                    cacheFrontLocked = false;
                }
            }
        }
    }
    
    pthread_exit(NULL);
}

void* quantizer(void* arg){
    while(1){
        if(!cache->isEmpty()){
            int l;
            
            pthread_mutex_lock(&cacheFront);
            
            if(cache->isEmpty()){//double check in critical section, exits if empty
                pthread_mutex_unlock(&cacheFront);
                continue;
            }
            
            double* frame = (double*)cache->top_front();
            
            for(l=0;l<8;l++)//quantization
                frame[l] = (frame[l]<=0.5 ? 0.0 : 1.0);
            
            sleep(3);
            
            for(l=0;l<8;l++)//print
                cout<<setprecision(1)<<fixed<<frame[l]<<(l<7 ? " " : "\n");
            
            cache->pop_front();
            
            pthread_mutex_unlock(&cacheFront);
        }
        else{//if empty
            int cnt=0;
            
            while(cnt<3){
                sleep(1);
                
                if(!cache->isEmpty())
                    break;
                
                cnt++;
            }
            
            if(cnt>=3)
                break;
        }
    }
    
    pthread_exit(NULL);
}

int main(int argc, const char * argv[]) {
    int interval, c, q1, q2;
    pthread_t cam, qnt1, qnt2;
    cache = new Queue();
    
    cin>>interval;
    if(cin.fail()){
        cout<<"invalid input...\n";
        exit(EXIT_FAILURE);
    }
    
    c = pthread_create(&cam, NULL, camera, &interval);
    q1 = pthread_create(&qnt1, NULL, quantizer, NULL);
    q2 = pthread_create(&qnt2, NULL, quantizer, NULL);
    
    if(c||q1||q2){
        cout<<"thread creation error...\n";
        exit(EXIT_FAILURE);
    }
    
    c = pthread_join(cam, NULL);
    q1 = pthread_join(qnt1, NULL);
    q2 = pthread_join(qnt2, NULL);
    
    if(c||q1||q2){
        cout<<"thread joining error...\n";
        exit(EXIT_FAILURE);
    }
    
    delete cache;
    
    //pthread_exit(NULL);
    return 0;
}
