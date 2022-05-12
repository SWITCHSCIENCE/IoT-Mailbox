package main

import (
	"flag"
	"fmt"
	"io"
	"log"
	"sync"

	"github.com/tarm/serial"
)

type Count struct {
	sync.RWMutex
	Up   int
	Down int
}

var count = &Count{}

func (c *Count) IncUp() {
	c.Lock()
	c.Up++
	c.Unlock()
}

func (c *Count) IncDown() {
	c.Lock()
	c.Down++
	c.Unlock()
}

func (c *Count) Print() {
	c.RLock()
	fmt.Printf("\rUp: %d, Down: %d", c.Up, c.Down)
	c.RUnlock()
}

func pass(fn func(), s1 io.Writer, s2 io.Reader) error {
	b := []byte{0}
	for {
		if _, err := s2.Read(b); err != nil {
			return err
		}
		if _, err := s1.Write(b); err != nil {
			return err
		}
		fn()
	}
}

func main() {
	baudRate := flag.Int("baud", 115200, "Baud rate")
	flag.Parse()
	s1, err := serial.OpenPort(&serial.Config{
		Name: flag.Arg(0),
		Baud: *baudRate,
	})
	if err != nil {
		log.Fatal(err)
	}
	defer s1.Close()
	s2, err := serial.OpenPort(&serial.Config{
		Name: flag.Arg(1),
		Baud: *baudRate,
	})
	if err != nil {
		log.Fatal(err)
	}
	defer s2.Close()
	/*
		go pass(count.IncUp, s1, s2)
		go pass(count.IncDown, s2, s1)
		tick := time.NewTicker(3 * time.Second)
		for {
			<-tick.C
			count.Print()
		}
	*/
	go io.Copy(s1, s2)
	go io.Copy(s2, s1)
	select {}
}
