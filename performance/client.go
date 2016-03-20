package main

import (
	"net"
	"strconv"
	"sync"

	"github.com/adambalogh/bchat_perf"
	pb "github.com/golang/protobuf/proto"
)

func length(size int) [4]byte {
	var out [4]byte
	out[0] = byte((size >> 24) & 0xff)
	out[1] = byte((size >> 16) & 0xff)
	out[2] = byte((size >> 8) & 0xff)
	out[3] = byte(size & 0xff)
	return out
}

func main() {
	total_messages := 1000000

	workers := 5000
	msg_per_worker := total_messages / workers

	var wg sync.WaitGroup
	wg.Add(workers)

	for i := 0; i < workers; i++ {
		go run(i, msg_per_worker, &wg)
	}

	wg.Wait()
}

func run(index int, messages int, wg *sync.WaitGroup) {

	name := strconv.Itoa(index)
	socket, err := net.Dial("tcp", "localhost:7002")
	if err != nil {
		panic(err)
	}

	var req_type proto.Request_Type
	req_type = proto.Request_Authentication

	req := proto.Request{
		Type: &req_type,
		Authentication: &proto.Authentication{
			Name: &name,
		},
	}

	data, err := pb.Marshal(&req)
	prefix := length(len(data))

	socket.Write(prefix[:])
	socket.Write(data)

	content := "Hello"
	t := proto.Request_Message
	req = proto.Request{
		Type: &t,
		Message: &proto.Message{
			Recipient: &name,
			Content:   &content,
		},
	}

	data, err = pb.Marshal(&req)
	prefix = length(len(data))

	go func() {
		var recv [10000]byte
		socket.Read(recv[:])
	}()

	for i := 0; i < messages; i++ {
		socket.Write(prefix[:])
		socket.Write(data)
	}

	wg.Done()
}
