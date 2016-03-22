package main

import (
	"bytes"
	"encoding/binary"
	"flag"
	"fmt"
	"os"
	"os/exec"
	"runtime"
	"strconv"
	"sync"

	"github.com/adambalogh/gochat/proto"
	pb "github.com/golang/protobuf/proto"
)

// Size converts an int to a byte array.
//
// size must fit in 4 bytes
func Size(size int) []byte {
	out := make([]byte, 4)
	out[0] = byte(size & 0xff)
	out[1] = byte((size >> 1) & 0xff)
	out[2] = byte((size >> 2) & 0xff)
	out[3] = byte((size >> 3) & 0xff)
	return out
}

func main() {
	runtime.GOMAXPROCS(4)

	numClients := flag.Int("num_clients", 1, "number of clients")
	flag.Parse()

	var wg sync.WaitGroup
	wg.Add(*numClients)

	for i := 0; i < *numClients; i++ {
		go runTCPKali(i, &wg)
	}

	wg.Wait()
}

func runTCPKali(i int, wg *sync.WaitGroup) {
	defer wg.Done()

	name := strconv.Itoa(i)

	// Authentication Message
	authType := proto.Request_Authentication

	authReq := proto.Request{}
	authReq.Type = &authType
	authReq.Authentication = &proto.Authentication{
		Name: &name,
	}
	authData, err := pb.Marshal(&authReq)
	if err != nil {
		fmt.Errorf("%s\n", err)
		return
	}

	authFileName := fmt.Sprintf("auth-%s", name)
	authFile, err := os.Create(authFileName)
	if err != nil {
		fmt.Errorf("%s\n", err)
		return
	}
	defer authFile.Close()
	header := make([]byte, 4)
	binary.BigEndian.PutUint32(header, uint32(len(authData)))
	authFile.Write(header)
	authFile.Write(authData)

	// Message Message
	msgType := proto.Request_Message
	content := "rghiogthw345tieoghrguhfleihuwihfergleirhgrw"

	msgReq := proto.Request{}
	msgReq.Type = &msgType
	msgReq.Message = &proto.Message{
		Recipient: &name,
		Content:   &content,
	}
	msgData, err := pb.Marshal(&msgReq)
	if err != nil {
		fmt.Errorf("%s\n", err)
		return
	}

	msgFileName := fmt.Sprintf("msg-%s", name)
	msgFile, err := os.Create(msgFileName)
	if err != nil {
		fmt.Errorf("%s\n", err)
		return
	}
	defer msgFile.Close()
	binary.BigEndian.PutUint32(header, uint32(len(msgData)))
	msgFile.Write(header)
	msgFile.Write(msgData)

	// Execute tcpkali
	cmd := exec.Command("tcpkali",
		"--nagle", "on",
		"--connections", "1",
		"--verbose", "1",
		"--first-message-file", authFileName,
		"--message-file", msgFileName,
		"localhost:7002",
	)

	var out bytes.Buffer
	cmd.Stdout = &out
	cmd.Stderr = &out

	err = cmd.Run()
	if err != nil {
		fmt.Println(out.String())
		fmt.Errorf("%s\n", err)
		return
	}

	fmt.Println(out.String())

	// Remove message files
	err = os.Remove(authFileName)
	if err != nil {
		fmt.Errorf("%s\n", err)
		return
	}
	err = os.Remove(msgFileName)
	if err != nil {
		fmt.Errorf("%s\n", err)
		return
	}
}
