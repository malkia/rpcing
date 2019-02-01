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
    commandRequest.set_command( command );

    grpc::ClientContext clientContext;
    //clientContext.set_compression_algorithm(GRPC_COMPRESS_DEFLATE);

    grpc::Status status = m_stub->Command( &clientContext, commandRequest, &commandResponse );

    if( status.ok() )
    {
      return commandResponse.result();
    }

    printf("failed with %d, %s\n", status.error_code(), status.error_message().c_str() );
    return std::string();
  }
};

std::string toastedString{ "Toasted!" };

class CommandService : public service::MainControl::Service {
  grpc::Status Command( grpc::ServerContext* context, const service::CommandRequest *request, service::CommandResponse *response )
  {
     //context->set_compression_algorithm( GRPC_COMPRESS_DEFLATE );
     response->set_result( toastedString );
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
  std::string serverAddr{ "0.0.0.0:56789" };

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
  for( int i=0; i<8192; i++ )
  {
    auto reply = commandClient.Command( testString );
    //printf( "%d, [%s]\n", counter++, reply.c_str() );
    //std::this_thread::sleep_for( std::chrono::seconds(1) );
  }
  auto t2 = std::chrono::steady_clock::now();
  printf( "%10.4f\n", (t2-t1).count()/1e9);
  return 0;
}
