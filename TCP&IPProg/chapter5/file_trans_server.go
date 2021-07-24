package server

import (
	"fmt"
	"io"
	"log"
	"net"
	"os"
)

func StartFileTransServer(localAddr string) {
	laddr, err := net.ResolveTCPAddr("tcp", localAddr)
	if err != nil {
		log.Fatalln(err)
	}
	listener, err := net.ListenTCP("tcp", laddr)
	if err != nil {
		log.Fatalln(err)
	}
	defer listener.Close()

	for {
		conn, err := listener.Accept()
		if err != nil {
			log.Println(err)
			continue
		}
		go handleFileTransConn(conn)
	}
}

func handleFileTransConn(conn net.Conn) {
	defer conn.Close()

	fmt.Printf("received connection from %s\n", conn.RemoteAddr())
	sizeBuf := make([]byte, 1)
	conn.Read(sizeBuf)
	size := int(sizeBuf[0])
	filenameBuf := make([]byte, size)
	conn.Read(filenameBuf)
	filename := string(filenameBuf)

	f, err := os.OpenFile(filename, os.O_RDONLY, 0666)
	if err != nil {
		log.Println(err)
		conn.Write([]byte{0x00})
		return
	}
	defer f.Close()

	conn.Write([]byte{0x01})
	io.Copy(conn, f)
}
