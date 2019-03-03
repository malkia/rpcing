package main

import (
	"context"
	"flag"
	"io"
	"log"
	"net"
	"sync/atomic"
	"time"

	service "github.com/malkia/rpcing"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/metadata"
	"google.golang.org/grpc/status"
)

var listen = flag.String("listen", "127.0.0.1:56789", "Listens an grpc endpoint.")
var connect = flag.String("connect", "127.0.0.1:56789", "Connects to an grpc endpoint.")
var listenMillis = flag.Int("listen_ms", 1000, "Listen time in millisecons.")
var connections = flag.Int("connections", 5, "Number of simultaneous clients.")
var calls = flag.Int("calls", 10, "Number of sequential calls per client.")
var messages = flag.Int("messages", 40, "Number of messages (send+recv) per call.")

type Counters struct {
	serverRecvTotal  int32
	serverRecvEOF    int32
	serverRecvFailed int32
	serverSendTotal  int32
	serverSendFailed int32

	serverSetTrailerCount   int32
	serverGetMetadataTotal  int32
	serverGetMetadataFailed int32
	serverSendHeaderTotal   int32
	serverSendHeaderFailed  int32

	clientNewStreamTotal  int32
	clientNewStreamFailed int32

	clientRecvTotal  int32
	clientRecvFailed int32
	clientRecvEOF    int32

	clientSendTotal  int32
	clientSendFailed int32
	clientSendEOF    int32

	clientHeaderTotal  int32
	clientHeaderFailed int32
	clientHeaderEOF    int32

	clientTrailerCount int32
}

var counters = &Counters{}

func printCounters() {
	if *connect != "" {
		log.Printf("client new stream total    %v", atomic.LoadInt32(&counters.clientNewStreamTotal))
		log.Printf("client new stream failed   %v", atomic.LoadInt32(&counters.clientNewStreamFailed))
		log.Print("---")
		log.Printf("client recv total          %v", atomic.LoadInt32(&counters.clientRecvTotal))
		log.Printf("client recv failed         %v", atomic.LoadInt32(&counters.clientRecvFailed))
		log.Printf("client recv EOF            %v", atomic.LoadInt32(&counters.clientRecvEOF))
		log.Print("---")
		log.Printf("client send total          %v", atomic.LoadInt32(&counters.clientSendTotal))
		log.Printf("client send failed         %v", atomic.LoadInt32(&counters.clientSendFailed))
		log.Printf("client send EOF            %v", atomic.LoadInt32(&counters.clientSendEOF))
		log.Print("---")
		log.Printf("client header total        %v", atomic.LoadInt32(&counters.clientHeaderTotal))
		log.Printf("client header failed       %v", atomic.LoadInt32(&counters.clientHeaderFailed))
		log.Printf("client header EOF          %v", atomic.LoadInt32(&counters.clientHeaderEOF))
		log.Print("---")
		log.Printf("client trailer count       %v", atomic.LoadInt32(&counters.clientTrailerCount))
	}

	if *listen != "" {
		log.Printf("server recv total          %v", atomic.LoadInt32(&counters.serverRecvTotal))
		log.Printf("server recv failed         %v", atomic.LoadInt32(&counters.serverRecvFailed))
		log.Printf("server recv EOF            %v", atomic.LoadInt32(&counters.serverRecvEOF))
		log.Printf("server send total          %v", atomic.LoadInt32(&counters.serverSendTotal))
		log.Printf("server send failed         %v", atomic.LoadInt32(&counters.serverSendFailed))
		log.Print("---")
		log.Printf("server set trailer count   %v", atomic.LoadInt32(&counters.serverSetTrailerCount))
		log.Printf("server get metadata total  %v", atomic.LoadInt32(&counters.serverGetMetadataTotal))
		log.Printf("server get metadata failed %v", atomic.LoadInt32(&counters.serverGetMetadataFailed))
		log.Printf("server send header total   %v", atomic.LoadInt32(&counters.serverSendHeaderTotal))
		log.Printf("server send header failed  %v", atomic.LoadInt32(&counters.serverSendHeaderFailed))
		log.Print("---")
	}
}

type serverService struct{}

func (s *serverService) Command(ctx context.Context, in *service.CommandRequest) (*service.CommandResponse, error) {
	log.Fatal("NYI")
	return nil, nil
}

func (s *serverService) CommandStreamOut(in *service.CommandRequest, stream service.MainControl_CommandStreamOutServer) error {
	log.Fatal("NYI")
	return nil
}

func (s *serverService) CommandStreamIn(stream service.MainControl_CommandStreamInServer) error {
	log.Fatal("NYI")
	return nil
}

func (s *serverService) CommandStream(stream service.MainControl_CommandStreamServer) error {
	y := service.CommandResponse{}

	defer func() {
		trailer := metadata.Pairs("malkiaHeader", "malkiaValue")
		atomic.AddInt32(&counters.serverSetTrailerCount, 1)
		stream.SetTrailer(trailer)
	}()

	atomic.AddInt32(&counters.serverGetMetadataTotal, 1)
	md, ok := metadata.FromIncomingContext(stream.Context())
	if !ok {
		atomic.AddInt32(&counters.serverGetMetadataFailed, 1)
		return status.Errorf(codes.DataLoss, "CommandStream: failed to get metadata")
	}

	atomic.AddInt32(&counters.serverSendHeaderTotal, 1)
	if e := stream.SendHeader(md); e != nil {
		atomic.AddInt32(&counters.serverSendHeaderFailed, 1)
		return e
	}

	for {

		atomic.AddInt32(&counters.serverRecvTotal, 1)
		_, e := stream.Recv()
		if e == io.EOF {
			atomic.AddInt32(&counters.serverRecvEOF, 1)
			return nil
		}
		if e != nil {
			atomic.AddInt32(&counters.serverRecvFailed, 1)
			return e
		}

		atomic.AddInt32(&counters.serverSendTotal, 1)
		if e := stream.Send(&y); e != nil {
			atomic.AddInt32(&counters.serverSendFailed, 1)
			return e
		}
	}
	return nil
}

func main() {
	flag.Parse()

	// server
	//	serverChan := make(chan struct{})
	var server *grpc.Server
	if *listen != "" {
		//		log.Printf("server: --listen=%v", *listen)
		serverStarted := make(chan struct{})
		go func() {
			//			log.Print("server(gofunc): listening...")
			listener, err := net.Listen("tcp", *listen)
			if err != nil {
				log.Fatalf("failed to listen: %v", err)
			}
			//			log.Print("server(gofunc): new server...")
			server = grpc.NewServer()
			//			log.Print("server(gofunc): register...")
			service.RegisterMainControlServer(server, &serverService{})
			//			log.Print("server(gofunc): server...")
			close(serverStarted)
			if err := server.Serve(listener); err != nil {
				log.Fatalf("failed to serve: %v", err)
			}
			//			log.Print("server(gofunc): stopped...")
		}()
		//		log.Print("server: waiting to start...")
		<-serverStarted
		//		log.Print("server: started!")
	}

	var clientChans []chan struct{}
	if *connect != "" {
		clientChans = make([]chan struct{}, *connections)
		for i := range clientChans {
			clientChans[i] = make(chan struct{})
		}

		ctx, cancel := context.WithTimeout(context.Background(), time.Second*100)
		defer cancel()

		conn, err := grpc.Dial(*connect, grpc.WithInsecure())
		if err != nil {
			log.Fatalf("error connecting to %v", err)
		}
		defer conn.Close()

		channel := service.NewMainControlClient(conn)

		for clientIndex := range clientChans {
			//index2 := clientIndex
			clientChan := clientChans[clientIndex]
			//log.Printf("client: channel %v", clientIndex)
			go func() {
				//log.Printf("client(gofunc): channel %v %v", clientIndex, index2)
				commandRequest := service.CommandRequest{}
				for callIndex := int(0); callIndex < *calls; callIndex++ {
					// https://godoc.org/google.golang.org/grpc#ClientConn.NewStream
					/*
						To ensure resources are not leaked due to the stream returned,
						one of the following actions must be performed:

						1. Call Close on the ClientConn.
						2. Cancel the context provided.
						3. Call RecvMsg until a non-nil error is returned. A protobuf-generated
						   client-streaming RPC, for instance, might use the helper function
						   CloseAndRecv (note that CloseSend does not Recv, therefore is not
						   guaranteed to release all resources).
						4. Receive a non-nil, non-io.EOF error from Header or SendMsg.
						   If none of the above happen, a goroutine and a context will be leaked,
						   and grpc will not call the optionally-configured stats handler with a stats.End message.
					*/
					atomic.AddInt32(&counters.clientNewStreamTotal, 1)
					stream, err := channel.CommandStream(ctx)
					if err != nil {
						atomic.AddInt32(&counters.clientNewStreamFailed, 1)
						continue
					}

					headerEOF := false

					atomic.AddInt32(&counters.clientHeaderTotal, 1)
					_, e := stream.Header()
					if e == io.EOF {
						atomic.AddInt32(&counters.clientHeaderEOF, 1)
						headerEOF = true
					} else if e != nil {
						atomic.AddInt32(&counters.clientHeaderFailed, 1)
						continue
					}

					var sendError error
					//var recvError error

					for messageIndex := int(0); messageIndex < *messages; messageIndex++ {
						atomic.AddInt32(&counters.clientSendTotal, 1)
						sendError = stream.Send(&commandRequest)
						if headerEOF && sendError != io.EOF {
							log.Fatalf("got headerEOF, but Send() returned %v", sendError)
						}
						if sendError == io.EOF {
							atomic.AddInt32(&counters.clientSendEOF, 1)
						} else if sendError != nil {
							atomic.AddInt32(&counters.clientSendFailed, 1)
							break
						}

						atomic.AddInt32(&counters.clientRecvTotal, 1)
						_, e := stream.Recv()
						if e != nil {
							//recvError = e
							if e == io.EOF {
								atomic.AddInt32(&counters.clientRecvEOF, 1)
							} else {
								atomic.AddInt32(&counters.clientRecvFailed, 1)
								break
							}
						}
					}

					//if recvError != nil {
					//	_ := stream.Trailer()
					//}

				}
				//				log.Printf("client(gochan): about to close %v", index2)
				close(clientChan)
				//				log.Printf("client(gochan): close %v", index2)
			}()
		}
	}

	if *listen != "" {
		//		log.Print("about to stop")
		time.Sleep(time.Duration(*listenMillis) * time.Millisecond)
		//		log.Print("about to stop2")
		server.Stop()
		//server.Stop()
	}

	if *connect != "" {
		for i := range clientChans {
			<-clientChans[i]
		}
	}

	printCounters()
}
