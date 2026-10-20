# The town server. Built from the repository root (see /railway.json):
#   docker build -f server/Dockerfile .
FROM golang:1.24-alpine AS build
WORKDIR /src
COPY server/go.mod ./
COPY server/*.go ./
RUN CGO_ENABLED=0 go build -trimpath -ldflags="-s -w" -o /server .

FROM scratch
COPY --from=build /server /server
# Mount a persistent volume here (Railway: service > Volumes, mount path /data).
ENV DATA_DIR=/data
EXPOSE 8080
ENTRYPOINT ["/server"]
