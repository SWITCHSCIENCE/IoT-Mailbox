package main

import (
	"bufio"
	"context"
	"flag"
	"log"
	"os"
	"os/signal"
	"strconv"
	"strings"
	"syscall"
	"time"

	"github.com/brutella/hc"
	"github.com/brutella/hc/accessory"
	hclog "github.com/brutella/hc/log"
	"github.com/brutella/hc/service"
	"github.com/tarm/serial"
	qrcode "github.com/yeqown/go-qrcode"
)

type State int

const (
	BOOT State = iota
	_
	EMPTY
	DETECT
)

func (s State) String() string {
	switch s {
	case BOOT:
		return "BOOT"
	case EMPTY:
		return "EMPTY"
	case DETECT:
		return "DETECT"
	}
	return "unknown"
}

type MyTime time.Time

func (t MyTime) String() string {
	return time.Time(t).Format(time.RFC3339)
}

type Record struct {
	Time    MyTime
	ID      string
	RSSI    int
	SNR     int
	Battery float64
	State   State
}

type gateway struct {
	id   string
	port *serial.Port
	acc  *PostSensor
}

func (g *gateway) scanner(recv chan<- Record) {
	defer close(recv)
	scan := bufio.NewScanner(g.port)
	for scan.Scan() {
		line := scan.Text()
		if strings.HasPrefix(line, "RX:") {
			params := strings.Split(line, "\t")
			rec := Record{Time: MyTime(time.Now())}
			fail := false
			for _, p := range params[1:] {
				kv := strings.SplitN(p, "=", 2)
				switch kv[0] {
				case "rssi":
					i, err := strconv.Atoi(kv[1])
					if err != nil {
						log.Print(err)
						fail = true
					}
					rec.RSSI = i
				case "snr":
					i, err := strconv.Atoi(kv[1])
					if err != nil {
						log.Print(err)
						fail = true
					}
					rec.SNR = i
				case "id":
					rec.ID = kv[1]
				case "battery":
					f, err := strconv.ParseFloat(kv[1], 64)
					if err != nil {
						log.Print(err)
						fail = true
					}
					rec.Battery = f
				case "state":
					i, err := strconv.Atoi(kv[1])
					if err != nil {
						log.Print(err)
						fail = true
					}
					rec.State = State(i)
				}
			}
			if !fail {
				recv <- rec
			}
		}
	}
}

func (g *gateway) parser(ctx context.Context, recv <-chan Record) {
	for {
		select {
		case <-ctx.Done():
			return
		case v, ok := <-recv:
			if !ok {
				return
			}
			log.Println(v)
			if strings.EqualFold(v.ID, g.id) {
				switch v.State {
				case EMPTY:
					g.acc.State.ContactSensorState.SetValue(0)
				case DETECT:
					g.acc.State.ContactSensorState.SetValue(1)
				}
				g.acc.Battery.BatteryLevel.SetValue(int(100 * v.Battery / 3.0))
			}
		}
	}
}

func (g *gateway) proc(ctx context.Context) {
	ch := make(chan Record, 8)
	defer g.port.Close()
	go g.scanner(ch)
	g.parser(ctx, ch)
}

// PostSensor ...
type PostSensor struct {
	*accessory.Accessory
	State   *service.ContactSensor
	Battery *service.BatteryService
}

// NewDoor returns a Door state
func NewPost() *PostSensor {
	info := accessory.Info{
		Name: "Post",
	}
	acc := &PostSensor{}
	acc.Accessory = accessory.New(info, accessory.TypeSensor)
	acc.State = service.NewContactSensor()
	acc.State.ContactSensorState.SetValue(0)
	acc.Battery = service.NewBatteryService()
	acc.Battery.BatteryLevel.SetValue(100)
	acc.AddService(acc.State.Service)
	acc.AddService(acc.Battery.Service)
	return acc
}

func main() {
	var id string
	flag.StringVar(&id, "id", "73c19db9f3555d09", "device id(16 digit hex)")
	flag.Parse()
	if flag.NArg() != 1 {
		log.Fatal("need an argument what is name of serial-port")
	}
	hclog.Debug.Enable()
	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGHUP, syscall.SIGTERM)
	defer stop()

	acc := NewPost()
	t, err := hc.NewIPTransport(hc.Config{
		Port: "12345", Pin: "23000214",
	}, acc.Accessory)
	if err != nil {
		log.Fatal(err)
	}
	u, err := t.XHMURI()
	if err != nil {
		log.Fatal(err)
	}
	qrc, err := qrcode.New(u)
	if err != nil {
		log.Fatal(err)
	}
	if err := qrc.Save("qrcode.png"); err != nil {
		log.Fatal(err)
	}
	hc.OnTermination(func() {
		t.Stop()
	})
	go t.Start()

	port, err := serial.OpenPort(&serial.Config{Name: flag.Arg(0), Baud: 115200})
	if err != nil {
		log.Fatal(err)
	}
	g := &gateway{
		id:   id,
		port: port,
		acc:  acc,
	}
	go g.proc(ctx)

	<-ctx.Done()
	log.Print("terminated")
}
