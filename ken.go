package main

import (
	"context"
	"fmt"
	"io"
	"log"
	"net"
	"time"

	"github.com/golang/protobuf/proto"
	service "github.com/malkia/rpcing"
	"google.golang.org/grpc"
)

type server struct{}

func (s *server) Command(ctx context.Context, in *service.CommandRequest) (*service.CommandResponse, error) {
	log.Printf("Received: %v", in)
	return &service.CommandResponse{ResultId: 42}, nil
}

func (s *server) CommandStream(stream service.MainControl_CommandStreamServer) error {
	var x uint64 = 0
	y := service.CommandResponse{}
	for {
		r, e := stream.Recv()
		if e == io.EOF {
			return nil
			//			return stream.Send(&service.CommandResponse{ResultId: 13})
		}
		if e != nil {
			log.Printf("Recv Error: %v", e)
			return e
		}
		//log.Printf("server: %v", r)
		y.ResultId = x + r.CommandId
		if e := stream.Send(&y); e != nil {
			log.Printf("Send Error: %v", e)
			return e
		}
		//	x++
	}
	//log.Printf("Received: %v", in)
	//return &service.CommandResponse{ ResultId: 42}, nil
	return nil
}

func (s *server) CommandStreamOut(in *service.CommandRequest, stream service.MainControl_CommandStreamOutServer) error {
	log.Printf("C")
	//log.Printf("Received: %v", in)
	//return &service.CommandResponse{ ResultId: 42}, nil
	return nil
}

func (s *server) CommandStreamIn(stream service.MainControl_CommandStreamInServer) error {
	log.Printf("D")
	//log.Printf("Received: %v", )
	//    return &service.CommandResponse{ ResultId: 42}, nil
	return nil
}

func main() {
	listenAddr := ":50051"
	serveAddr := "localhost" + listenAddr
	networkType := "tcp"
	x := service.CommandRequest{}
	x.CommandId = 12
	y := service.CommandResponse{}
	data, err := proto.Marshal(&x)
	if err != nil {
		fmt.Println("%v", err)
	}
	var z service.CommandRequest
	err = proto.Unmarshal(data, &z)
	if err != nil {
		fmt.Println("%v", err)
	}
	fmt.Println("hello world %+v", x)
	fmt.Println("hello world %+v", y)
	fmt.Println("hello world %+v", z)
	a := service.MainControlServer(nil)
	b := service.MainControlClient(nil)
	fmt.Println("a=%+v", a)
	fmt.Println("b=%+v", b)

	go func() {
		lis, err := net.Listen(networkType, listenAddr)
		if err != nil {
			log.Fatalf("failed to listen: %v", err)
		}
		s := grpc.NewServer()
		service.RegisterMainControlServer(s, &server{})
		if err := s.Serve(lis); err != nil {
			log.Fatalf("failed to serve: %v", err)
		}
	}()

	ctx, cancel := context.WithTimeout(context.Background(), time.Second*100)
	defer cancel()

	conn, err := grpc.Dial(serveAddr, grpc.WithInsecure())
	if err != nil {
		log.Fatalf("error connecting to %v", err)
	}
	defer conn.Close()

	channel := service.NewMainControlClient(conn)
	r, err := channel.Command(ctx, &x)
	if err != nil {
		log.Fatalf("error calling %v", err)
	}
	//	log.Printf("hooray %v", r)

	var clientChans [100]chan struct{}
	for i := range clientChans {
		clientChans[i] = make(chan struct{})
	}
	for clientIndex := range clientChans {
		clientChan := clientChans[clientIndex]
		go func() {
			stream, err := channel.CommandStream(ctx)
			if err != nil {
				log.Fatalf("error calling %v", err)
			}
			//log.Printf("%v: hooray %v", clientIndex, stream)
			var total uint64 = 0
			for b := uint64(0); b <= 8192; b++ {
				//	for _, b := range []uint64{1, 2, 3, 4} {
				x.CommandId = b
				e := stream.Send(&x)
				if e != nil {
					log.Printf("stream.Send error %v", e)
				}
				r, e := stream.Recv()
				if e != nil {
					log.Printf("stream.Recv error %v", e)
				}
				total += r.ResultId
				//		log.Printf("stream.Recv: %v", r)
			}
			e := stream.CloseSend()
			if e != nil {
				log.Printf("CloseSend error %v", e)
			}
			if false {
				log.Printf("%v: last stream.Recv: %v", clientIndex, r)
			}
			close(clientChan)
		}()
	}
	for i := range clientChans {
		<-clientChans[i]
	}
}
