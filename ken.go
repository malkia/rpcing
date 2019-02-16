package main

import 
("log"
"fmt"
"time"
"context"
service "github.com/malkia/rpcing"
"github.com/golang/protobuf/proto"
"google.golang.org/grpc"
)
 
type server struct{}

func (s *server) SayHello(ctx context.Context, in *service.CommandRequest) (*service.CommandResponse, error) {
	log.Printf("Received: %v", in.Name)
	return &service.CommandResponse{ ResultId: 42}, nil
}

func main() {
    x := service.CommandRequest{}
    x.CommandId = 12;
    y := service.CommandResponse{}
    data, err := proto.Marshal(&x);
    if err != nil {
		fmt.Println("%v", err);
    }
    var z service.CommandRequest
	err = proto.Unmarshal(data, &z)
	if err != nil {
		fmt.Println("%v", err);
	}
    fmt.Println("hello world %+v", x)
    fmt.Println("hello world %+v", y)
    fmt.Println("hello world %+v", z)
    a := service.MainControlServer(nil)
	b := service.MainControlClient(nil)
    fmt.Println("a=%+v", a);
    fmt.Println("b=%+v", b);

    conn, err := grpc.Dial("localhost:50051", grpc.WithInsecure())
    if err != nil {
        log.Fatalf( "error connecting to %v", err );
    }
    defer conn.Close();

    channel := service.NewMainControlClient(conn);

    ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()

    r, err := channel.Command(ctx, &x);
    if err != nil {
        log.Fatalf( "error calling %v", err );
    }
    log.Printf( "hooray %v", r );
}
