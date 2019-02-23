#include "service.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <stdio.h>
#include <thread>
#include <chrono>
#include <assert.h>
#include <memory>
#include <unistd.h>
#include <string>
#include <future>
#include <vector>
#include <atomic>
#include <list>
#include <mutex>

struct Counter;
static std::list<Counter*> counters_;
static std::mutex counters_mutex_;

struct Counter
{
    std::atomic<size_t> counter_;
    const char* name_;
    const char* func_;
    const char* file_;
    int line_;
    explicit Counter( const char* name, const char* func, const char* file, int line )
        : counter_(0)
        , name_(name)
        , func_(func)
        , file_(file)
        , line_(line)
    {
        std::lock_guard<std::mutex> guard(counters_mutex_);
        counters_.push_back(this);
    }
};

#define COUNTER(n) do { static Counter counter(n, __FUNCTION__, __FILE__, __LINE__); counter.counter_++; } while( false )

struct PrintCounters
{
    ~PrintCounters()
    {
        for( auto counter : counters_ )
        {
            printf( "%s: %zd\n", counter->name_, counter->counter_.load() );
        }
    }
};

class CallData {
  grpc::ServerContext context_;
  grpc::ServerAsyncReaderWriter<service::CommandResponse, service::CommandRequest> stream_;
  service::MainControl::AsyncService* service_;
  service::CommandRequest request_;
  grpc::ServerCompletionQueue* cq_;

  enum class State : uint8_t {
    INITIAL,
    READ,
    WRITE,
    FINISHED
  };

  State state_;

public:
  CallData( service::MainControl::AsyncService *service, grpc::ServerCompletionQueue* cq )
    : stream_( &context_ )
    , service_( service )
    , cq_( cq )
    , state_( State::INITIAL )
  {
    COUNTER("CallData(Constructor)");
    service->RequestCommandStream( &context_, &stream_, cq_, cq_, this );
  }

  void Next() 
  {
    COUNTER("CallData::Next(beforSwitch)");
    switch( state_ ) 
    {
        default:
            COUNTER("CallData::Next(default) (SHOULD NOT HAPPEN!)");
            return;

        case State::FINISHED:
            COUNTER("CallData::Next(FINISHED)");
            delete this;
            return;

        case State::INITIAL:
            COUNTER("CallData::Next(INITIAL)");
            new CallData(service_, cq_);
            state_ = State::READ;
            stream_.SendInitialMetadata( this );
            return;

        case State::READ:
            COUNTER("CallData::Next(READ)");
            state_ = State::WRITE;
            stream_.Read( &request_, this );
            return;
  
        case State::WRITE:
            COUNTER("CallData::Next(WRITE)");
            state_ = State::READ;
            service::CommandResponse response;
            stream_.Write( response, this );
            return;
    }
    COUNTER("CallData::Next(ShouldNotBeHere) (SHOULD NOT HAPPEN!)");
  }

  void Stop()
  {
      COUNTER("CallData::Stop(beforeCheck)");
      if( state_ == State::INITIAL )      
      {
          COUNTER("CallData::Stop(INITIAL)");
          delete this;
          return;
      }
      else if( state_ == State::FINISHED )
      {
          COUNTER("CallData::Stop(FINISHED)");
          delete this;
          return;
      }
      else if( state_ == State::READ )
        COUNTER("CallData::Stop(READ)");
      else if( state_ == State::WRITE )
        COUNTER("CallData::Stop(WRITE)");
      else
        COUNTER("CallData::Stop(SHOULD NOT HAPPEN!)");
      
    state_ = State::FINISHED;
    stream_.Finish( grpc::Status::OK, this );
  }
};

struct BaseServer
{
    std::unique_ptr<grpc::Server> server_;
    std::unique_ptr<grpc::ServerCompletionQueue> cq_;
    service::MainControl::AsyncService service_;
    std::thread t;

    BaseServer( const std::string& addr )
    {
        printf("server about to start...\n");
        grpc::ServerBuilder serverBuilder;
        cq_ = serverBuilder.AddCompletionQueue();
        serverBuilder.AddListeningPort( addr, grpc::InsecureServerCredentials() );
        serverBuilder.RegisterService( &service_ );
        server_ = serverBuilder.BuildAndStart();
        printf("server started...\n");
        t = std::thread([this]{
            printf( "server start (begin)\n");

            void* tag; bool ok;
            new CallData( &service_, cq_.get() );
            while( cq_->Next( &tag, &ok ) )
            {
                COUNTER("ServerCQ(Next)");
                auto callData = (CallData*) tag;
                if( !ok )
                {
                    COUNTER("ServerCQ(Next).NotOK");
                    callData->Stop();
                }
                else
                {
                    COUNTER("ServerCQ(Next).OK");
                    callData->Next();
                }
            }
        });
    }

    ~BaseServer()
    {
        server_->Shutdown();
        cq_->Shutdown();
        for(;;) {
            COUNTER("ServerCQ(Drain).Before");
            void* ignored_tag = (void*) 1;
            bool ignored_ok;
            auto r = cq_->Next( &ignored_tag, &ignored_ok );
            printf( "server drain: r=%d ignored_ok=%d ignored_ta=%p\n", r, ignored_ok, ignored_tag );
            if( !r )
                break;
            COUNTER("ServerCQ(Drain).After");
        }
        t.join();
    }
};

struct Client
{
    std::string addr_;
    std::thread thread_;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<service::MainControl::Stub> stub_;
    int index_;

    explicit Client( const std::string& addr, int index )
        : addr_(addr), index_(index)
    {
        grpc::ChannelArguments channelArguments;
        channel_ = grpc::CreateCustomChannel(
          addr, grpc::InsecureChannelCredentials(), channelArguments
        );
        stub_ = service::MainControl::NewStub( channel_ );
    }

    void Call()
    {
        thread_ = std::thread([this]{
            
            grpc::ClientContext ctx;
            grpc::CompletionQueue cq;
            auto rpc = stub_->PrepareAsyncCommandStream( &ctx, &cq );

            void* tag; bool ok;

            COUNTER("Client(StartCall)");
            rpc->StartCall( (void*) -1 );
            if( !cq.Next( &tag, &ok ) || !ok || tag != (void*) -1 )
            {
                COUNTER("Client(StartCall).FAILURES");
                printf( "  client(%d) start call error: tag=%p ok=%d\n", index_, tag, ok );
                return;
            }

            COUNTER("Client(ReadInitialMetadata)");
            rpc->ReadInitialMetadata( (void*) -2 );
            if( !cq.Next( &tag, &ok ) || !ok || tag != (void*) -2 )
            {
                COUNTER("Client(ReadInitialMetadata).FAILURES");
                printf( "  client(%d) error: tag=%p ok=%d\n", index_, tag, ok );
                return;
            }
            
            //printf("  client: read initial metadata\n" );

            service::CommandRequest req;
            service::CommandResponse resp;

            for( int i=0; i<40; i++) 
            {
                COUNTER("Client(Write)");
                rpc->Write( req, (void*) -3 );
                if( !cq.Next( &tag, &ok ) || !ok || tag != (void*) -3 )
                {
                    COUNTER("Client(Write).FAILURES");
                    printf( "  client(%d) write error: tag=%p ok=%d\n", index_, tag, ok );
                    return;
                }
        
                COUNTER("Client(Read)");
                rpc->Read( &resp, (void*) -4 );
                if( !cq.Next( &tag, &ok ) || !ok || tag != (void*) -4 )
                {
                    COUNTER("Client(Read).FAILURES");
                    printf( "  client(%d) read error: tag=%p ok=%d\n", index_, tag, ok );
                    return;
                }
            }

            COUNTER("Client(WritesDone)");
            rpc->WritesDone( (void*) -5 );
            if( !cq.Next( &tag, &ok ) || !ok || tag != (void*) -5 )
            {
                COUNTER("Client(WritesDone).FAILURES");
                printf( "  client(%d) writes done error: tag=%p ok=%d\n", index_, tag, ok );
                return;
            }

            COUNTER("Client(Finish)");
            grpc::Status status;
            rpc->Finish( &status, (void*) -6 );
            if( !cq.Next( &tag, &ok ) || !ok || tag != (void*) -6 )
            {
                COUNTER("Client(Finish).FAILURES");
                printf( "  client(%d) finish error: tag=%p ok=%d\n", index_, tag, ok );
                return;
            }
            COUNTER("Client(FINISHED)");
        });
    }

    void Join()
    {
        COUNTER("Client(Join)");
        thread_.join();
    }

    void Join2()
    {
        COUNTER("Client(Join2)");
        thread_.join();
    }

    void Detach()
    {
        COUNTER("Client(Deatch)");
        thread_.detach();
    }
};

int main( int argc, const char* argv[] )
{
    std::string addr = "localhost:56789";
    std::vector<Client> clients;
    int clientCount = 1000;
    // Not really detached, just joining (Join2) them after we shutdown the server
    int detachedClients = clientCount / 3;
    {
        BaseServer server( addr );
        for( auto clientIndex = 0; clientIndex < clientCount; clientIndex++ )
            clients.emplace_back( addr, clientIndex );
        for( auto clientIndex = 0; clientIndex < clientCount; clientIndex++ )
            clients[clientIndex].Call();
        for( auto clientIndex = 0; clientIndex < clientCount-detachedClients; clientIndex++ )
            clients[clientIndex].Join();
        PrintCounters printCounters;
    }
    std::this_thread::sleep_for(std::chrono::seconds(10));
    for( auto clientIndex = clientCount-detachedClients; clientIndex < clientCount; clientIndex++ )
        clients[clientIndex].Join2();
    printf("\n");
    PrintCounters printCountersAfterJoin2;
}

