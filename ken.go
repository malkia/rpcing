package main

import "fmt"
import service "github.com/malkia/rpcing"
import "github.com/golang/protobuf/proto"

func main() {
    x := service.CommandRequest{}
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
	(&service.CommandRequest{}).Marshal()
    (&service.CommandResponse{}).Marshal()
    fmt.Println("a=%+v", a);
    fmt.Println("b=%+v", b);
}
