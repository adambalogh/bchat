package main

import (
	"bufio"
	"fmt"
	"net"
	"os"
	"strings"

	"github.com/adambalogh/gochat/proto"
	pb "github.com/golang/protobuf/proto"
)

func main() {
	c, err := NewClient()
	if err != nil {
		panic(err)
	}
	c.RecvLoop()

	c.Register("Adam")

	for {
		reader := bufio.NewReader(os.Stdin)
		command, _ := reader.ReadString('\n')

		parts := strings.Split(command, " ")
		if len(parts) != 2 {
			fmt.Println("Usage <name> <message>")
			return
		}

		c.SendMessage(parts[0], parts[1])
	}

}

type Client struct {
	conn net.Conn
}

func NewClient() (*Client, error) {
	c := Client{}
	conn, err := net.Dial("tcp", ":7002")
	if err != nil {
		return nil, err
	}
	c.conn = conn
	return &c, nil
}

func (c *Client) send(msg pb.Message) error {
	data, err := pb.Marshal(msg)
	if err != nil {
		return err
	}
	n, err := c.conn.Write(SizeEncode(len(data)))
	if err != nil {
		return err
	}
	n, err = c.conn.Write(data)
	if err != nil {
		return err
	}
	if n != len(data) {
		panic("Couldn't send msg in whole")
	}
	return nil
}

func (c *Client) Register(name string) error {
	authType := proto.Request_Authentication
	req := proto.Request{
		Type: &authType,
		Authentication: &proto.Authentication{
			Name: &name,
		},
	}
	return c.send(&req)
}

func (c *Client) SendMessage(recipient, message string) error {
	msgType := proto.Request_Message
	req := proto.Request{
		Type: &msgType,
		Message: &proto.Message{
			Recipient: &recipient,
			Content:   &message,
		},
	}
	return c.send(&req)
}

func (c *Client) DecodeResponse(data []byte) {
	res := proto.Response{}
	err := pb.Unmarshal(data, &res)
	if err != nil {
		panic(err)
	}
	fmt.Println(res)
}

func (c *Client) RecvLoop() {
	go func() {
		buf := make([]byte, 0)
		start := 0
		for {
			readBuf := make([]byte, 1000)
			n, err := c.conn.Read(readBuf)
			if err != nil {
				fmt.Errorf("Read err: %s", err)
				return
			}
			for i := 0; i < n; i++ {
				buf = append(buf, readBuf[i])
			}
			size := SizeDecode(buf[start : start+4])
			start += 4
			c.DecodeResponse(buf[start : start+size])
			start += n
		}
	}()
}

func SizeEncode(size int) []byte {
	out := make([]byte, 4)
	out[0] = byte((size >> 24) & 0xff)
	out[1] = byte((size >> 16) & 0xff)
	out[2] = byte((size >> 8) & 0xff)
	out[3] = byte((size >> 0) & 0xff)
	return out
}

func SizeDecode(bytes []byte) int {
	size := 0
	size += int(bytes[0] << 24)
	size += int(bytes[1] << 16)
	size += int(bytes[2] << 8)
	size += int(bytes[3])
	return size
}
