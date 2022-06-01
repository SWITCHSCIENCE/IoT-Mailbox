package main

import (
	"bufio"
	"context"
	"log"
	"net/http"
	"net/url"
	"os"
	"os/signal"
	"strconv"
	"strings"
	"syscall"
	"time"

	"github.com/caarlos0/env/v6"
	"github.com/tarm/serial"
)

type Config struct {
	Port        string        `env:"RECEIVER_PORT,notEmpty" envDefault:"/dev/ttyS0"`
	ID          string        `env:"TRANSMITTER_ID,notEmpty"`
	Token       string        `env:"LINE_NOTIFY_TOKEN,notEmpty"`
	OnMessage   string        `env:"ON_MESSAGE" envDefault:"何か届きました"`
	OffMessage  string        `env:"OFF_MESSAGE" envDefault:"何か届きました"`
	WaitTime    time.Duration `env:"WAIT_TIME" envDefault:"10s"`
	BatteryWarn float64       `env:"BATTERY_WARN" envDefault:"2.2"`
}

var cfg = Config{}

func init() {
	if err := env.Parse(&cfg); err != nil {
		log.Fatal(err)
	}
}

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

func (g *gateway) postMessage(ctx context.Context, msg string) {
	values := url.Values{}
	values.Set("message", msg)
	req, err := http.NewRequestWithContext(ctx, http.MethodPost, "https://notify-api.line.me/api/notify", strings.NewReader(values.Encode()))
	if err != nil {
		log.Print(err)
		return
	}
	req.Header.Set("Content-Type", "application/x-www-form-urlencoded")
	req.Header.Set("Authorization", "Bearer "+cfg.Token)
	req.Header.Set("User-Agent", "Line Notify")
	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		log.Print(err)
		return
	}
	defer resp.Body.Close()
	if resp.StatusCode != 200 {
		log.Print(resp.Status)
	}
}

func (g *gateway) parser(ctx context.Context, recv <-chan Record) {
	last := time.Now().Add(-cfg.WaitTime)
	for {
		select {
		case <-ctx.Done():
			return
		case v, ok := <-recv:
			if !ok {
				return
			}
			if time.Since(last) < cfg.WaitTime {
				log.Println(v, "->skip")
				continue
			}
			log.Println(v)
			if !strings.EqualFold(v.ID, cfg.ID) {
				continue
			}
			msg := ""
			switch v.State {
			default:
				continue
			case EMPTY:
				msg = cfg.OffMessage
			case DETECT:
				msg = cfg.OnMessage
			}
			if v.Battery < cfg.BatteryWarn {
				msg += "\n(バッテリー残量が低いです)"
			}
			g.postMessage(ctx, msg)
			last = time.Now()
		}
	}
}

func (g *gateway) proc(ctx context.Context) {
	ch := make(chan Record, 8)
	defer g.port.Close()
	go g.scanner(ch)
	g.parser(ctx, ch)
}

func main() {
	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGHUP, syscall.SIGTERM)
	defer stop()

	port, err := serial.OpenPort(&serial.Config{Name: cfg.Port, Baud: 115200})
	if err != nil {
		log.Fatal(err)
	}
	g := &gateway{
		id:   cfg.ID,
		port: port,
	}
	go g.proc(ctx)

	<-ctx.Done()
	log.Print("terminated")
}
