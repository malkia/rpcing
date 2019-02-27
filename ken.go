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
)

// DEFINE_bool(stats, true, "Print stats");
// DEFINE_string(listen, "127.0.0.1:56789", "Listens on grpc endpoint.");
// DEFINE_string(connect, "127.0.0.1:56789", "Connect to grpc endpoint.");
// DEFINE_int32(listen_ms, 1000, "Listen time in milliseconds");
// DEFINE_bool(detached, false, "Should detach all clients");

var listen = flag.String("listen", "127.0.0.1:56789", "Listens an grpc endpoint.")
var connect = flag.String("connect", "127.0.0.1:56789", "Connects to an grpc endpoint.")
var listenMillis = flag.Int("listen_ms", 1000, "Listen time in millisecons.")
var connections = flag.Int("connections", 50, "Number of simultaneous clients.")
var calls = flag.Int("calls", 10, "Number of sequential calls per client.")
var messages = flag.Int("messages", 40, "Number of messages (send+recv) per call.")

type Counters struct {
	serverRecvTotal  int32
	serverRecvEOF    int32
	serverRecvFailed int32
	serverSendTotal  int32
	serverSendFailed int32
	clientRecvTotal  int32
	clientRecvFailed int32
	clientSendTotal  int32
	clientSendFailed int32
}

var counters = &Counters{}

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

func printCounters() {
	if *connect != "" {
		log.Printf("client recv total  %v", atomic.LoadInt32(&counters.clientRecvTotal))
		log.Printf("client recv failed %v", atomic.LoadInt32(&counters.clientRecvFailed))
		log.Printf("client send total  %v", atomic.LoadInt32(&counters.clientSendTotal))
		log.Printf("client send failed %v", atomic.LoadInt32(&counters.clientSendFailed))
	}

	if *listen != "" {
		log.Printf("server recv total  %v", atomic.LoadInt32(&counters.serverRecvTotal))
		log.Printf("server recv failed %v", atomic.LoadInt32(&counters.serverRecvFailed))
		log.Printf("server recv EOF    %v", atomic.LoadInt32(&counters.serverRecvEOF))
		log.Printf("server send total  %v", atomic.LoadInt32(&counters.serverSendTotal))
		log.Printf("server send failed %v", atomic.LoadInt32(&counters.serverSendFailed))
	}
}

func main() {
	flag.Parse()

	// server
	//	serverChan := make(chan struct{})
	var server *grpc.Server
	if *listen != "" {
		go func() {
			listener, err := net.Listen("tcp", *listen)
			if err != nil {
				log.Fatalf("failed to listen: %v", err)
			}
			server = grpc.NewServer()
			service.RegisterMainControlServer(server, &serverService{})
			if err := server.Serve(listener); err != nil {
				log.Fatalf("failed to serve: %v", err)
			}
			//close(serverChan)
		}()
		//<-serverChan
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
			clientChan := clientChans[clientIndex]
			go func() {
				commandRequest := service.CommandRequest{}
				for callIndex := int(0); callIndex < *calls; callIndex++ {
					stream, err := channel.CommandStream(ctx)
					if err != nil {
						log.Fatalf("error calling %v", err)
					}
					for messageIndex := int(0); messageIndex < *messages; messageIndex++ {
						atomic.AddInt32(&counters.clientSendTotal, 1)
						if e := stream.Send(&commandRequest); e != nil {
							atomic.AddInt32(&counters.clientSendFailed, 1)
						}
						atomic.AddInt32(&counters.clientRecvTotal, 1)
						_, e := stream.Recv()
						if e != nil {
							atomic.AddInt32(&counters.clientRecvFailed, 1)
						}
					}
					e := stream.CloseSend()
					if e != nil {
						log.Printf("CloseSend error %v", e)
					}
					if false {
						log.Printf("%v: last stream.Recv", clientIndex)
					}
				}
				close(clientChan)
			}()
		}
	}

	if *listen != "" {
		time.Sleep(time.Millisecond * time.Duration(*listenMillis))
		server.GracefulStop()
		//server.Stop()
	}

	if *connect != "" {
		for i := range clientChans {
			<-clientChans[i]
		}
	}

	printCounters()
}
