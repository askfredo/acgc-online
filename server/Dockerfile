# Small static image: the server is a single Go binary with no dependencies.
FROM golang:1.24-alpine AS build
WORKDIR /src
COPY go.mod ./
COPY *.go ./
RUN CGO_ENABLED=0 go build -trimpath -ldflags="-s -w" -o /server .

FROM scratch
COPY --from=build /server /server
# Mount a persistent volume here (Railway: service > Volumes, mount path /data).
ENV DATA_DIR=/data
EXPOSE 8080
ENTRYPOINT ["/server"]
