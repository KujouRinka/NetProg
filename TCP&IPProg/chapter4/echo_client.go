package server

import (
	"io"
	"log"
	"net"
	"os"
	"sync"
)

func StartEchoClient() {
	raddr, err := net.ResolveTCPAddr("tcp", ":8888")
	if err != nil {
		log.Fatalln(err)
	}
	server, err := net.DialTCP("tcp", nil, raddr)
	if err != nil {
		log.Fatalln(err)
	}
	var wg sync.WaitGroup
	wg.Add(1)
	go io.Copy(server, os.Stdin)
	go func() {
		io.Copy(os.Stdout, server)
		wg.Done()
	}()
	wg.Wait()
}
