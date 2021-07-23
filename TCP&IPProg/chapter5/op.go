package server

import (
	"bufio"
	"bytes"
	"encoding/binary"
	"fmt"
	"log"
	"net"
	"os"
	"strconv"
)

func intToBytes(n int) []byte {
	x := int32(n)
	bytesBuffer := bytes.NewBuffer([]byte{})
	binary.Write(bytesBuffer, binary.BigEndian, x)
	return bytesBuffer.Bytes()
}

func bytesToInt(b []byte) int {
	bytesBuffer := bytes.NewBuffer(b)

	var x int32
	binary.Read(bytesBuffer, binary.BigEndian, &x)
	return int(x)
}

func StartNewOpServer() {
	laddr, err := net.ResolveTCPAddr("tcp", ":8888")
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
		go handleOpConn(conn)
	}
}

func handleOpConn(conn net.Conn) {
	defer conn.Close()

	operBuf := make([]byte, 1)
	_, err := conn.Read(operBuf)
	operand := int(operBuf[0])
	fmt.Printf("operand gotten: %d\n", operand)
	if err != nil {
		log.Println(err)
	}
	rawDataBuf := make([]byte, 4)
	var dataBuf []int

	for i := 0; i < operand; i++ {
		_, err := conn.Read(rawDataBuf)
		if err != nil {
			log.Println(err)
		}
		dataBuf = append(dataBuf, bytesToInt(rawDataBuf))
	}

	operBuf = make([]byte, 1)
	_, err = conn.Read(operBuf)
	if err != nil {
		log.Println(err)
	}
	operator := string(operBuf)

	result := calculate(dataBuf, operator)

	fmt.Println(result)
	_, err = conn.Write(intToBytes(result))
	if err != nil {
		log.Println(err)
	}
}

func calculate(data []int, operator string) int {
	base := data[0]
	data = data[1:]
	for _, n := range data {
		switch operator {
		case "+":
			base += n
		case "-":
			base -= n
		case "*":
			base *= n
		}
	}
	return base
}

func StartNewOpClient() {
	raddr, err := net.ResolveTCPAddr("tcp", ":8888")
	if err != nil {
		log.Fatalln(err)
	}
	conn, err := net.DialTCP("tcp", nil, raddr)
	if err != nil {
		log.Fatalln(err)
	}
	defer conn.Close()

	var data []byte
	scanner := bufio.NewScanner(os.Stdin)
	fmt.Printf("Operand count: ")
	scanner.Scan()
	operand, _ := strconv.Atoi(scanner.Text())
	data = append(data, byte(operand))
	for i := 0; i < operand; i++ {
		fmt.Printf("Operand %d: ", i+1)
		scanner.Scan()
		n, _ := strconv.Atoi(scanner.Text())

		data = append(data, intToBytes(n)...)
	}
	fmt.Printf("Operator: ")
	scanner.Scan()
	operator := scanner.Text()
	data = append(data, []byte(operator)...)
	n, err := conn.Write(data)
	if err != nil {
		log.Println(err)
	}
	fmt.Printf("%d bytes data written.\n", n)

	result := make([]byte, 4)
	n, err = conn.Read(result)
	if err != nil {
		log.Println(err)
	}
	fmt.Printf("%d bytes data recveived.\n", n)
	fmt.Printf("Result: %d\n", bytesToInt(result))
}
