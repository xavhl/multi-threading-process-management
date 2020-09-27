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
    void push_back(void*);
    void* top_front();
    void* pop_front();
    //int size;
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
    cout<<"destruction finished...\n";
}

bool Queue::isEmpty(){return front==NULL;}

bool Queue::isFull(){return size>=MAXIMUM_FRAME_CAP;}

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
static pthread_mutex_t cacheMtx = PTHREAD_MUTEX_INITIALIZER;

double* generate_frame_vector(int l);

void* camera(void *arg){
    int interval = *(int*)arg;
    
    while(1){
        sleep(interval);
        
        if(cache->isFull())
            continue;
        else{
            double* frame_vector = generate_frame_vector(4*2);
            
            if(!frame_vector)
                break;
            else{
                pthread_mutex_lock(&cacheMtx); cout<<"camera locked mutex\n";
                cache->push_back(frame_vector); cout<<"camera pushed cache ";
                for(int l=0;l<8;l++){
			//cout<<setprecision(1)<<fixed<<frame_vector[l]<<(l<7 ? " " : "\n");
			cout<<(frame_vector[l]<=0.5?0:1)<<(l<7 ? " " : "\n");
		}
                pthread_mutex_unlock(&cacheMtx); cout<<"camera unlocked mutex\n";
            }
        }
    }
    cout<<"camera exited...\n";
    pthread_exit(NULL);
}

void* quantizer(void* arg){
    while(1){
        if(!cache->isEmpty()){
            int l;
            pthread_mutex_lock(&cacheMtx); cout<<"quantizer "<<*(int*)arg<<" locked mutex\n";
            if(cache->isEmpty()){
                pthread_mutex_unlock(&cacheMtx); cout<<"quantizer"<<*(int*)arg<<" unlocked mutex\n";
                continue;
            }
            
            double* frame = (double*)cache->top_front();
            
            for(l=0;l<8;l++)
                frame[l] = (frame[l]<=0.5 ? 0.0 : 1.0);
            
            sleep(3);
            cout<<"quantizer"<<*(int*)arg<<" ";
            cout<<"quantized cache ";
            for(l=0;l<8;l++)
                cout<<setprecision(1)<<fixed<<frame[l]<<(l<7 ? " " : "\n");
            
            cache->pop_front();
            pthread_mutex_unlock(&cacheMtx); cout<<"quantizer"<<*(int*)arg<<" unlocked mutex\n";
        }
        else{
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
    //cout<<"quantization completed...\n";
    cout<<"quantizer"<<*(int*)arg<<" exited...\n";
    pthread_exit(NULL);
}

int main(int argc, const char * argv[]) {
    int interval, c, q1, q2;
    pthread_t cam, qnt1, qnt2;
    cache = new Queue();
    int id[] = {1,2};
    
    cin>>interval;
    if(cin.fail()){
        cout<<"invalid input...\n";
        exit(EXIT_FAILURE);
    }
    
    c = pthread_create(&cam, NULL, camera, &interval);
    cout<<"camera created "<<c<<endl;
    q1 = pthread_create(&qnt1, NULL, quantizer, &(id[0]));
    cout<<"quantizer1 created "<<q1<<endl;
    q2 = pthread_create(&qnt2, NULL, quantizer, &(id[1]));
    cout<<"quantizer2 created "<<q2<<endl;
    
    if(c||q1||q2){
        cout<<"thread creation error...\n";
        exit(EXIT_FAILURE);
    }
    
    c = pthread_join(cam, NULL);
    cout<<"camera joined "<<c<<endl;
    q1 = pthread_join(qnt1, NULL);
    cout<<"quantizer1 joined "<<q1<<endl;
    q2 = pthread_join(qnt2, NULL);
    cout<<"quantizer2 joined "<<q2<<endl;
    
    if(c||q1||q2){
        cout<<"thread joining error...\n";
        exit(EXIT_FAILURE);
    }
    
    delete cache;
    
    cout<<"main exited...\n";
    //pthread_exit(NULL);
    return 0;
}
