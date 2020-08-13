package main

import (
	"fmt"
	"net/http"

	"github.com/badfortrains/mdns"
)

func main() {
	fmt.Println("Dupa")
	info := []string{"VERSION=1.0", "CPath=/"}
	http.HandleFunc("/", HelloServer)

	service, _ := mdns.NewMDNSService("dupa", "asdasd", "", "", 5151, nil, info)
	server, _ := mdns.NewServer(&mdns.Config{Zone: service})
	defer server.Shutdown()
	http.ListenAndServe(":12515", nil)
}

func HelloServer(w http.ResponseWriter, r *http.Request) {
	fmt.Println("OI")
	fmt.Fprintf(w, `{"status":101,"statusString":"OK","spotifyError":0,"version":"2.7.1","libraryVersion":"1.2.2","accountReq":"PREMIUM","brandDisplayName":"librespot-org","modelDisplayName":"cpot","voiceSupport":"NO","availability":"","productID":0,"tokenType":"default","groupStatus":"NONE","resolverVersion":"0","scope":"streaming,client-authorization-universal","activeUser":"","deviceID":"t8s2ogzatcbtgmkyrlqpagjkbc3e5d4ucbgnhlun","remoteName":"dddspot","publicKey":"6HBKO1dTcEA4XYJpoyJES0azVBSgXeRcZBRRWzJwM7rsJfiWOiHRuSlEe/ObW84yRx0mEKhnxXOIg1GpVV3jeaK/1o0H0YQvSV8sKrrMdV0zL3LvdBmLIUIeccAGyhql","deviceType":"COMPUTER"}`)
}
