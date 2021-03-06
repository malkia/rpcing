#include "service.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <gflags/gflags.h>

#include <assert.h>
#include <stdio.h>
//#include <unistd.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <memory>
#include <cstdlib>

DEFINE_bool(stats, true, "Print stats");
DEFINE_string(listen, "127.0.0.1:56789", "Listens on grpc endpoint.");
DEFINE_string(connect, "127.0.0.1:56789", "Connect to grpc endpoint.");
DEFINE_int32(listen_ms, 1000, "Listen time in milliseconds");
DEFINE_bool(detached, false, "Should detach all clients");

DEFINE_int32(connections, 50, "Number of connections.");
DEFINE_int32(calls, 10, "Number of calls per connection.");
DEFINE_int32(messages, 40, "Number of messagesper call.");

enum {
    COUNTER_PERIODS = 8,
};

struct Counter;
static std::atomic<int> counter_period_;
static std::vector<Counter*> counters_;
static std::mutex counters_mutex_;

struct Counter
{
    std::atomic<size_t> counter_[COUNTER_PERIODS];
    const char* name_;
    const char* func_;
    const char* file_;
    int line_;
    explicit Counter( const char* name, const char* func, const char* file, int line )
        : name_(name)
        , func_(func)
        , file_(file)
        , line_(line)
    {
        std::lock_guard<std::mutex> guard(counters_mutex_);
        counters_.push_back(this);
    }
};

#define COUNTER(n) do { static Counter counter(n, __FUNCTION__, __FILE__, __LINE__); counter.counter_[counter_period_]++; } while( false )

void NextCounterPeriod()
{
    counter_period_++;
    assert(counter_period_<COUNTER_PERIODS);
}

static void MySleep(int amount)
{
//    std::this_thread::sleep_for(std::chrono::milliseconds(rand()%10==0?1:0));
//    std::this_thread::sleep_for(std::chrono::milliseconds(rand()%5));
//    std::this_thread::sleep_for(std::chrono::milliseconds(rand()%(1+rand()%(rand()%(amount/10+1)+1))));
}

struct PrintCounters
{
    ~PrintCounters()
    {
        std::stable_sort( counters_.begin(), counters_.end(), []( Counter* a, Counter* b ) -> bool {
            return std::string(a->name_) < std::string(b->name_);
        });
        for( auto counter : counters_ )
        {
            printf( "%-36.36s ", counter->name_ );
            size_t total = 0;
            for( auto period = 0; period <= counter_period_; period++ )
            {
                size_t counter_value = counter->counter_[period].load();
                printf( "%8zd ", counter_value );
                total += counter_value;
            }
            printf( "|%8zd\n", total );
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
    COUNTER("CallData::CallData().TOTAL");
    service->RequestCommandStream( &context_, &stream_, cq_, cq_, this );
  }

  void Next() 
  {
    COUNTER("CallData::Next().TOTAL");
    switch( state_ ) 
    {
        case State::INITIAL:
            COUNTER("CallData::Next(1_INITIAL)");
            new CallData(service_, cq_);
            state_ = State::READ;
            stream_.SendInitialMetadata( this );
            break;

        case State::READ: {
            COUNTER("CallData::Next(2_READ)");
            state_ = State::WRITE;
            MySleep(500);
            stream_.Read(&request_, this);
            break;
        }
  
        case State::WRITE:
            COUNTER("CallData::Next(3_WRITE)");
            state_ = State::READ;
            {
                service::CommandResponse response;
                response.set_allocated_request(new service::CommandRequest(request_));
                MySleep(500);
                stream_.Write( response, this );
            }
            break;

        case State::FINISHED:
            COUNTER("CallData::Next(4_FINISHED)");
            delete this;
            break;

        default:
            assert(0);
            break;
    }
  }

  void Stop()
  {
    COUNTER("CallData::Stop().TOTAL");
    bool delete_this = false;

    switch( state_ )
    {
        case State::INITIAL:
            COUNTER("CallData::Stop(1_INITIAL)");
            delete_this = true;
            break;

        case State::READ:
            COUNTER("CallData::Stop(2_READ)");
            break;

        case State::WRITE:
            COUNTER("CallData::Stop(3_WRITE)");
            break;

        case State::FINISHED:
            COUNTER("CallData::Stop(4_FINISHED)");
            delete_this = true;
            break;

        default:
            assert(0);
            break;
    }
      
    if( delete_this )
    {
        delete this;
        return;
    }

    state_ = State::FINISHED;
    stream_.Finish( grpc::Status::OK, this );
  }
};

struct BaseServer
{
    std::unique_ptr<grpc::Server> server_;
    std::unique_ptr<grpc::ServerCompletionQueue> cq_;
    service::MainControl::AsyncService service_;
    std::thread polling_thread_;

    BaseServer( const std::string& addr )
    {
        printf("server: Starting...\n");
        grpc::ServerBuilder serverBuilder;
        cq_ = serverBuilder.AddCompletionQueue();
        serverBuilder.AddListeningPort( addr, grpc::InsecureServerCredentials() );
        serverBuilder.RegisterService( &service_ );
        server_ = serverBuilder.BuildAndStart();
        polling_thread_ = std::thread([this]{
            printf("  server: Polling...\n");
            void* tag; bool ok;
            new CallData( &service_, cq_.get() );
            while( cq_->Next( &tag, &ok ) )
            {
                COUNTER("PollCQ(Next).TOTAL");
                auto callData = (CallData*) tag;
                if( !ok )
                {
                    COUNTER("PollCQ(Next).NOT_OK");
                    callData->Stop();
                    continue;
                }
                callData->Next();
            }
            printf("  server: Finished polling...\n");
        });
    }

    ~BaseServer()
    {
        printf("server: Shutting down...\n");
        server_->Shutdown();
        printf("server: Shut down! Draining the queue...\n");
        cq_->Shutdown();
        printf("server: Waiting to drain...\n");
        polling_thread_.join();
        printf("server: Drained the queue! Finished...\n");
    }
};

struct Client
{
    std::string addr_;
    std::thread thread_;
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<service::MainControl::Stub> stub_;
    uint32_t index_;

    explicit Client( const std::string& addr, uint32_t index )
        : addr_(addr), index_(index)
    {
        grpc::ChannelArguments channelArguments;
        channel_ = grpc::CreateCustomChannel(
          addr_, grpc::InsecureChannelCredentials(), channelArguments
        );
        stub_ = service::MainControl::NewStub( channel_ );
    }

    void Call()
    {
        thread_ = std::thread([this]{
            auto callCount = FLAGS_calls;
            auto messageCount = FLAGS_messages;

            service::CommandRequest req;
            req.mutable_implementation_details()->set_programming_language( service::ImplementationDetails_ProgrammingLanguage_CC );
            req.set_connection_index(index_);

          grpc::CompletionQueue cq;
          for(uint32_t callIndex=0;callIndex<callCount;callIndex++) {
            grpc::ClientContext ctx;

            MySleep(1000);

            auto rpc = stub_->PrepareAsyncCommandStream( &ctx, &cq );

            void* tag; bool ok;

            MySleep(1000);

            COUNTER("Client(1_StartCall).TOTAL");
            rpc->StartCall( (void*) -1 );
            tag = (void*) 1;
            if( !cq.Next( &tag, &ok ) )
            {
                assert( tag == (void*) 1 );
                COUNTER("Client(1_StartCall).CANCELED");
                return;
            }
            assert( tag == (void*) -1 );
            if( !ok )
            {
                COUNTER("Client(1_StartCall).NOT_OK");
                return;
            }

            MySleep(1000);

            COUNTER("Client(2_ReadInitialMetadata).TOTAL");
            rpc->ReadInitialMetadata( (void*) -2 );
            tag = (void*) 2;
            if( !cq.Next( &tag, &ok ) )
            {
                assert( tag == (void*) 2 );
                COUNTER("Client(2_ReadInitialMetadata).CANCELED");
                return;
            }
            assert( tag == (void*) -2 );
            if( !ok )
            {
                COUNTER("Client(2_ReadInitialMetadata).NOT_OK");
                return;
            }
            
            service::CommandResponse resp;

            MySleep(1000);

            req.set_call_index(callIndex);

            for( uint32_t messageIndex=0; messageIndex<messageCount; messageIndex++)
            {
                req.set_message_index(messageIndex);
                COUNTER("Client(3_Write).TOTAL");
                rpc->Write( req, (void*) -3 );
                tag = (void*) 3;
                if( !cq.Next( &tag, &ok ) )
                {
                    assert( tag == (void*) 3 );
                    COUNTER("Client(3_Write).CANCELED");
                    return;
                }
                assert( tag == (void*) -3 );
                if( !ok )
                {
                    COUNTER("Client(3_Write).NOT_OK");
                    return;
                }

                MySleep(1000/40);  

                COUNTER("Client(4_Read).TOTAL");
                rpc->Read( &resp, (void*) -4 );
                tag = (void*) 4;
                if( !cq.Next( &tag, &ok ) )
                {
                    assert( tag == (void*) 4 );
                    COUNTER("Client(4_Read).CANCELED");
                    return;
                }
                assert( tag == (void*) -4 );
                if( !ok )
                {
                    COUNTER("Client(4_Read).NOT_OK");
                    return;
                }

                auto respConnectionIndex = resp.request().connection_index();
                auto respCallIndex = resp.request().call_index();
                auto respMessageIndex = resp.request().message_index();

                if( respConnectionIndex != index_
                ||  respCallIndex != callIndex
                ||  respMessageIndex != messageIndex )
                {
                    printf( " ERROR: resp=[%d %d %d], expecting=[%d %d %d]\n", respConnectionIndex, respCallIndex, respMessageIndex, index_, callIndex, messageIndex );
                }

                MySleep(1000/40);  
            }

            MySleep(1000);  

            COUNTER("Client(5_WritesDone).TOTAL");
            rpc->WritesDone( (void*) -5 );
            tag = (void*) 5;
            if( !cq.Next( &tag, &ok ) )
            {
                assert( tag == (void*) 5 );
                COUNTER("Client(5_WritesDone).CANCELED");
                return;
            }
            assert( tag == (void*) -5 );
            if( !ok )
            {
                COUNTER("Client(5_WritesDone).NOT_OK");
                return;
            }

            MySleep(1000);  

            COUNTER("Client(6_Finish).TOTAL");
            grpc::Status status;
            rpc->Finish( &status, (void*) -6 );
            tag = (void*) 6;
            if( !cq.Next( &tag, &ok ) )
            {
                assert( tag == (void*) 6);
                COUNTER("Client(6_Finish).CANCELED");
                return;
            }
            assert( tag == (void*) -6 );
            if( !ok )
            {
                COUNTER("Client(6_Finish).NOT_OK");
                return;
            }
        }});
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

int main( int argc, char* argv[] ) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    std::unique_ptr<BaseServer> server;
    if( !FLAGS_listen.empty() )
        server = std::unique_ptr<BaseServer>( new BaseServer( FLAGS_listen ) );

    std::vector<Client> clients;
    int clientCount = FLAGS_connections;
    clients.reserve(clientCount);
    int detachedClients = FLAGS_detached ? clientCount : 0;

    if( !FLAGS_connect.empty() )
    {
        for( auto clientIndex = 0; clientIndex < clientCount; clientIndex++ )
            clients.emplace_back( FLAGS_connect, clientIndex );
       for( auto clientIndex = 0; clientIndex < clientCount; clientIndex++ )
            clients[clientIndex].Call();
        NextCounterPeriod();
        MySleep(10);
        for( auto clientIndex = 0; clientIndex < clientCount-detachedClients; clientIndex++ )
            clients[clientIndex].Join();
        NextCounterPeriod();
        MySleep(10);
    }

    if( !FLAGS_listen.empty() )
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(FLAGS_listen_ms));
        server.reset();
    }
    
    if( !FLAGS_connect.empty() )
    {
        NextCounterPeriod();
        MySleep(10);
        for( auto clientIndex = clientCount-detachedClients; clientIndex < clientCount; clientIndex++ )
            clients[clientIndex].Join2();
    }

    if( FLAGS_stats )
    {
        PrintCounters printCountersAfterJoin2;
    }

    fflush(stdout);
    fflush(stderr);

    // Avoid expensive deallocation & destruction.
    std::quick_exit(0);
}

