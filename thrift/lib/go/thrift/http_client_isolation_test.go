/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package thrift

import (
	"io"
	"net/http"
	"net/http/httptest"
	"testing"
	"time"
)

func TestHTTPClientSettingsAreIndependent(t *testing.T) {
	first, err := newHTTPPostClient("http://localhost/")
	if err != nil {
		t.Fatal(err)
	}
	second, err := newHTTPPostClient("http://localhost/")
	if err != nil {
		t.Fatal(err)
	}
	if first.client == second.client || first.client == http.DefaultClient {
		t.Fatal("HTTP clients share mutable settings")
	}
	defaultTimeout := http.DefaultClient.Timeout
	first.client.Timeout = defaultTimeout + time.Second
	if second.client.Timeout != defaultTimeout || http.DefaultClient.Timeout != defaultTimeout {
		t.Fatal("changing one client's timeout changed another client")
	}
	if first.client.Transport != http.DefaultClient.Transport {
		t.Fatal("HTTP client did not preserve the default transport")
	}
}

func TestHTTPClientIndependentSettingsRoundTrip(t *testing.T) {
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		io.Copy(w, r.Body)
	}))
	defer server.Close()
	client, err := newHTTPPostClient(server.URL)
	if err != nil {
		t.Fatal(err)
	}
	defer client.Close()
	client.client.Timeout = time.Second
	if _, err := client.WriteString("request"); err != nil {
		t.Fatal(err)
	}
	if err := client.Flush(); err != nil {
		t.Fatal(err)
	}
	response := make([]byte, len("request"))
	if _, err := io.ReadFull(client, response); err != nil {
		t.Fatal(err)
	}
	if string(response) != "request" {
		t.Fatalf("unexpected response: %q", response)
	}
}
