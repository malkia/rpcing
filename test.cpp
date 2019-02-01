#include "service.grpc.pb.h"

#include <grpcpp/grpcpp.h>

#include <stdio.h>
#include <thread>
#include <chrono>

class CommandClient {
  std::unique_ptr<service::MainControl::Stub> m_stub;
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
};

std::string toastedString{ "Toasted!" };

class CommandService : public service::MainControl::Service {
  grpc::Status Command( grpc::ServerContext* context, const service::CommandRequest *request, service::CommandResponse *response ) override
  {
     //context->set_compression_algorithm( GRPC_COMPRESS_DEFLATE );
     //response->set_result( toastedString );
     return grpc::Status::OK;
  }

  grpc::Status CommandStream( grpc::ServerContext* context, grpc::ServerReaderWriter<service::CommandResponse, service::CommandRequest>* stream ) override
  {
     service::CommandRequest commandRequest;
     service::CommandResponse commandResponse;
     while( stream->Read(&commandRequest) )
     {
       //commandResponse.set_result( toastedString );
       if( !stream->Write(commandResponse) )
         printf("w! !");
     }
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

  grpc::ServerBuilder serverBuilder;
  //serverBuilder.SetDefaultCompressionAlgorithm(GRPC_COMPRESS_GZIP);
  serverBuilder.AddListeningPort( serverAddr, grpc::InsecureServerCredentials() );
  CommandService theService;
  serverBuilder.RegisterService( &theService );
  std::thread( serverThreadProc, std::move(serverBuilder.BuildAndStart()) ).detach();

  grpc::ChannelArguments channelArguments;
  //channelArguments.SetCompressionAlgorithm(GRPC_COMPRESS_GZIP);
  CommandClient commandClient(grpc::CreateCustomChannel(
    serverAddr, grpc::InsecureChannelCredentials(), channelArguments
  ));

  //int counter = 0;
  std::string testString{ "test" };
  auto t1 = std::chrono::steady_clock::now();
#if 1
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
