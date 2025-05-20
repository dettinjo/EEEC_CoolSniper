package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"io/ioutil"
	"net/http"
	"os"
	"strconv"
	"strings"
)

type PredictionResponse struct {
	IsThrottled  []bool    `json:"is_throttled"`
	Probability  []float64 `json:"probability"`
}

func predictThrottle(data string) (int, error) {
	url := "http://yyds.nl:8000/predict"
	payload := map[string]string{"data": data}
	jsonPayload, err := json.Marshal(payload)
	if err != nil {
		return 0, err
	}

	resp, err := http.Post(url, "application/json", strings.NewReader(string(jsonPayload)))
	if err != nil {
		return 0, err
	}
	defer resp.Body.Close()

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return 0, err
	}

	var result PredictionResponse
	err = json.Unmarshal(body, &result)
	if err != nil {
		return 0, err
	}
	sum := 0
	for _, v := range result.IsThrottled {
		if v {
			sum += 1
		}
	}
	if sum >= 2 {
		return 1, nil
	}
	return 0, nil
}

func main() {
	dataPtr := flag.String("data", "", "Comma-separated string of 16 values")
	flag.Parse()

	if *dataPtr == "" {
		fmt.Println("Error: --data flag is required")
		os.Exit(1)
	}

	dataList := strings.Split(*dataPtr, ",")
	if len(dataList) != 16 {
		fmt.Println("Error: Input should be exactly 16 comma-separated values")
		os.Exit(1)
	}

	for i, v := range dataList {
		_, err := strconv.ParseFloat(v, 64)
		if err != nil {
			fmt.Printf("Error: Invalid float value at position %d: %s\n", i, v)
			os.Exit(1)
		}
	}

	result, err := predictThrottle(*dataPtr)
	if err != nil {
		fmt.Printf("Error: %v\n", err)
		os.Exit(1)
	}

	fmt.Printf("%+v\n", result)
}
