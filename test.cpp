#include "service.grpc.pb.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/resource_quota.h>

#include <stdio.h>
#include <thread>
#include <chrono>
#include <assert.h>
#include <memory>

gpr_timespec grpc_deadline_in_millis( uint64_t deadlineInMillis )
{
  return gpr_time_add(
      gpr_now(GPR_CLOCK_MONOTONIC),
      gpr_time_from_micros( deadlineInMillis * 1000, GPR_TIMESPAN )
  );
}

struct ContextWithResponse
{
  std::unique_ptr<grpc::ClientContext> context;
  service::CommandResponse response;
};

struct RequestWithResponse
{
  service::CommandRequest request;
  service::CommandResponse response;
};

class CommandClient {
  std::unique_ptr<service::MainControl::Stub> m_stub;
  grpc::CompletionQueue m_cq;
public:
  CommandClient( std::shared_ptr<grpc::Channel> channel )
    : m_stub( service::MainControl::NewStub( channel ) )
  {
     printf("CommandClient\n");
  }

  std::string Command( const std::string& command )
  {
    service::CommandResponse commandResponse;
    service::CommandRequest commandRequest;
    //commandRequest.set_command( command );

    grpc::ClientContext clientContext;
    //clientContext.set_compression_algorithm(GRPC_COMPRESS_DEFLATE);

    grpc::Status status = m_stub->Command( &clientContext, commandRequest, &commandResponse );

    if( status.ok() )
    {
      return std::string(); //commandResponse.result();
    }

    printf("failed with %d, %s\n", status.error_code(), status.error_message().c_str() );
    return std::string();
  }

  void CommandStream( size_t batchSize )
  {
      grpc::ClientContext clientContext;
      service::CommandRequest commandRequest;
      service::CommandResponse commandResponse;

      auto stream = m_stub->CommandStream( &clientContext );
      for( size_t index = 0; index < batchSize; index++ )
      {
        if( !stream->Write(commandRequest) )
          printf( "W! " );

        if( !stream->Read(&commandResponse) )
          printf( "R! " );
      }

      if( !stream->WritesDone() )
         printf( "\nWD!\n" );
      auto status = stream->Finish();
      if( !status.ok() )
      {
        printf( "CommandStream error: %d, %s\n", status.error_code(), status.error_message().c_str() );
      }
  }


  grpc::Status IssueAsyncCommand( const service::CommandRequest& request, ContextWithResponse& contextWithResponse, grpc::CompletionQueue& completionQueue )
  {
    auto rpc{ m_stub->PrepareAsyncCommand( contextWithResponse.context.get(), request, &completionQueue ) };
    rpc->StartCall();
    grpc::Status status;
    rpc->Finish( &contextWithResponse.response, &status, (void*)&contextWithResponse );
    return status;
  }

  grpc::Status IssueAsyncCommandStream( 
    grpc::ClientContext& clientContext, 
    std::vector<RequestWithResponse>& requestsWithResponses, 
    grpc::CompletionQueue& completionQueue )
  {
    auto rpc{ m_stub->PrepareAsyncCommandStream( &clientContext, &completionQueue ) };
    bool ok = false;
    void* tag = nullptr;
    printf("1\n");
    rpc->StartCall( (void*) -1 );
    if( !completionQueue.Next(&tag, &ok) || !ok || tag != (void*) -1 )
       printf("failed, tag=%p\n", tag );
    printf("2\n");
    rpc->ReadInitialMetadata( (void*) -2 );
    if( !completionQueue.Next(&tag, &ok) || !ok || tag != (void*) -2 )
       printf("failed, tag=%p\n", tag );
    printf("2a\n");
    // For each Write, a tag must be received from the completion queue, before procedding with another.
    for( auto& requestWithResponse : requestsWithResponses )
    {
      rpc->Write( requestWithResponse.request, (void*)&requestWithResponse.request );
      if( !completionQueue.Next(&tag, &ok) || !ok || tag != (void*)&requestWithResponse.request )
        printf("failed, tag=%p\n", tag );
      rpc->Read( &requestWithResponse.response, (void*)&requestWithResponse.response );
      if( !completionQueue.Next(&tag, &ok) || !ok || tag != (void*)&requestWithResponse.response )
        printf("failed, tag=%p\n", tag );
    }
    printf("6\n");
    rpc->WritesDone((void*)-3);
    printf("7\n");
    if( !completionQueue.Next(&tag, &ok) || !ok || tag != (void*) -3 )
       printf("failed, tag=%p\n", tag );
    printf("8\n");
    grpc::Status status;
    rpc->Finish( &status, (void*) -4 );
    printf("9\n");
    if( !completionQueue.Next(&tag, &ok) || !ok || tag != (void*) -4 )
       printf("failed, tag=%p\n", tag );
    printf("10\n");
    return status;
  }

  grpc::CompletionQueue::NextStatus ProcessAsyncCommands( size_t batchSize, grpc::CompletionQueue& cq, size_t deadlineInMillis )
  {
    const auto deadline{ grpc_deadline_in_millis(deadlineInMillis) };
    bool keepGoing = true;
    size_t processedCount = 0;
    for( ; processedCount < batchSize; processedCount ++ )
    {
      void* tag = nullptr;
      bool ok = false;
      auto status = cq.AsyncNext( &tag, &ok, deadline ); 
      switch( status )
      {
        case grpc::CompletionQueue::NextStatus::SHUTDOWN:
          printf("\nSHUTDOWN\n"); keepGoing = false; break;
        break;
        case grpc::CompletionQueue::NextStatus::TIMEOUT:
          printf("\nTIMEOUT\n"); keepGoing = false; break;
        break;
        case grpc::CompletionQueue::NextStatus::GOT_EVENT:
        processedCount++;
        break;
        default:
          assert(0);
          break;
      }
      if( !keepGoing )
        break;
      if( !ok )
        printf( "tag=%p, ok=%d\n", tag, ok );
    }
    printf( "processed count=%zd\n", processedCount );
    return grpc::CompletionQueue::NextStatus::GOT_EVENT;
  }

  void AsyncCommand( const std::string& command )
  {
    grpc::Status status;
    grpc::ClientContext clientContext;
    grpc::CompletionQueue completionQueue;
    service::CommandRequest commandRequest;
    service::CommandResponse commandResponse;
    auto rpc{ m_stub->PrepareAsyncCommand( &clientContext, commandRequest, &m_cq ) };
    rpc->StartCall();
    rpc->ReadInitialMetadata( (void*) 0 );
    rpc->Finish( &commandResponse, &status, (void*) 1 );
    void* tag = nullptr;
    bool ok = false;
    bool hasNext = true;
    while( hasNext )
    {
      for(int i=0;i<128;i++) { printf("."); fflush(stdout); std::this_thread::sleep_for( std::chrono::milliseconds(rand()%100) ); }
      switch( m_cq.AsyncNext( &tag, &ok, grpc_deadline_in_millis(10000) ) )
      {
        case grpc::CompletionQueue::NextStatus::SHUTDOWN:
          printf("\nSHUTDOWN\n"); return;
        break;
        case grpc::CompletionQueue::NextStatus::TIMEOUT:
          printf("\nTIMEOUT\n"); return;
        break;
        case grpc::CompletionQueue::NextStatus::GOT_EVENT:
        break;
      }
      printf( "tag=%p ok=%d next=%d\n", tag, ok, hasNext );
      if( !status.ok() )
      {
        printf( "AsyncCommand error: %d, %s\n", status.error_code(), status.error_message().c_str() );
      }
    }
  }
};

std::string toastedString{ "Toasted!" };

class CommandService : public service::MainControl::Service {
  grpc::Status Command( grpc::ServerContext* context, const service::CommandRequest *request, service::CommandResponse *response ) override
  {
    //context->set_compression_algorithm( GRPC_COMPRESS_DEFLATE );
    //response->set_result( toastedString );
    //for(int i=0;i<128;i++) { printf("!"); fflush(stdout); std::this_thread::sleep_for( std::chrono::milliseconds(rand()%100) ); }
    return grpc::Status::OK;
  }

  grpc::Status CommandStream( grpc::ServerContext* context, grpc::ServerReaderWriter<service::CommandResponse, service::CommandRequest>* stream ) override
  {
     service::CommandRequest commandRequest;
     service::CommandResponse commandResponse;
     printf(" A'\n");
     stream->SendInitialMetadata();
     size_t readCount = 0;
     size_t writeCount = 0;
     for( ;; )
     {
     //   printf(" A\n");
        if( !stream->Read(&commandRequest) )
          break;
        readCount++;
   //     printf(" B\n");
       //commandResponse.set_result( toastedString );
      if( !stream->Write(commandResponse) )
         printf(" w! !");
      writeCount++;
     }
     printf(" C readCount=%zd writeCount=%zd\n", readCount, writeCount );
     return grpc::Status::OK;
  }
};

static void serverThreadProc( std::unique_ptr<grpc::Server> server )
{
  printf("About to start\n");
  server->Wait();
  printf("About to finish\n");
}

int main( int argc, const char* argv[] )
{
  std::string serverAddr{ "localhost:56789" };

  grpc::ResourceQuota serverResourceQuota{ "serverResourceQuota" };
  grpc::ResourceQuota clientResourceQuota{ "clientResourceQuota" };

  serverResourceQuota.SetMaxThreads( 16 );
  serverResourceQuota.Resize( 1024*1024*16 );//* 1024 * 1024 );
  clientResourceQuota.SetMaxThreads( 16 );
  clientResourceQuota.Resize( 1024*1024*16 );//* 1024 * 1024 );

  grpc::ServerBuilder serverBuilder;
  serverBuilder.SetResourceQuota( serverResourceQuota );
  serverBuilder.SetDefaultCompressionAlgorithm(GRPC_COMPRESS_NONE);
  serverBuilder.AddListeningPort( serverAddr, grpc::InsecureServerCredentials() );
  CommandService theService;
  serverBuilder.RegisterService( &theService );
  std::thread( serverThreadProc, std::move(serverBuilder.BuildAndStart()) ).detach();

  grpc::ChannelArguments channelArguments;
  channelArguments.SetResourceQuota( clientResourceQuota );
  channelArguments.SetCompressionAlgorithm(GRPC_COMPRESS_NONE);
  CommandClient commandClient(grpc::CreateCustomChannel(
    serverAddr, grpc::InsecureChannelCredentials(), channelArguments
  ));

  //int counter = 0;
  std::string testString{ "test" };
  auto t1 = std::chrono::steady_clock::now();
#if 1
  grpc::CompletionQueue completionQueue;
  grpc::ClientContext clientContext;
  service::CommandRequest request;
  size_t batchSize = 16384*4;
  std::vector<RequestWithResponse> requestsWithResponses;
  auto t10 = std::chrono::steady_clock::now();
  requestsWithResponses.resize( batchSize );
  printf( "construction        time=%20.8f\n", (t10-t1).count()/1e9);
  commandClient.IssueAsyncCommandStream( clientContext, requestsWithResponses, completionQueue );
  auto t11 = std::chrono::steady_clock::now();
  printf( "init                time=%20.8f\n", (t11-t10).count()/1e9);
#elif 1
  grpc::CompletionQueue completionQueue;
  service::CommandRequest request;
  size_t batchSize = 16384*4;
  std::vector<CommandClient::ContextWithResponse> contextsWithResponses;
  auto t10 = std::chrono::steady_clock::now();
  printf( "construction        time=%20.8f\n", (t10-t1).count()/1e9);
  contextsWithResponses.resize( batchSize );
  for( auto& contextWithResponse : contextsWithResponses )
    contextWithResponse.context = std::unique_ptr<grpc::ClientContext>(new grpc::ClientContext);
  auto t11 = std::chrono::steady_clock::now();
  printf( "init                time=%20.8f\n", (t11-t10).count()/1e9);
  for( auto& contextWithResponse : contextsWithResponses )
    commandClient.IssueAsyncCommand( request, contextWithResponse, completionQueue );
  auto t12 = std::chrono::steady_clock::now();
  printf( "issueAsyncCommands  time=%20.8f\n", (t12-t11).count()/1e9);
  commandClient.ProcessAsyncCommands( contextsWithResponses.size(), completionQueue, 10000 );
  auto t13 = std::chrono::steady_clock::now();
  printf( "processAsyncCommands time=%20.8f\n", (t13-t12).count()/1e9);
#elif 1
  commandClient.AsyncCommand("");
#elif 1
  commandClient.CommandStream(120000);
#else
  for( int i=0; i<8192; i++ )
  {
    auto reply = commandClient.Command( testString );
    //printf( "%d, [%s]\n", counter++, reply.c_str() );
    //std::this_thread::sleep_for( std::chrono::seconds(1) );
  }
#endif
  auto t2 = std::chrono::steady_clock::now();
  printf( "%20.8f\n", (t2-t1).count()/1e9);
  return 0;
}
