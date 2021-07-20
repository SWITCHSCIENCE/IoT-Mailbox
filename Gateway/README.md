# Serial2HomeKit Gateway

HomeKit アクセサリの接続 QR コードの作り方

```go
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
```
