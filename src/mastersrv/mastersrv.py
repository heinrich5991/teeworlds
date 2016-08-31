from flask import Flask, abort, jsonify, request
from redis import StrictRedis
import socket

app = Flask(__name__)
redis = StrictRedis()
udp = socket.socket(type=socket.SOCK_DGRAM)

EXPIRE_SECONDS=90 # Time until a server without heartbeats is removed.
KEY_SERVER="server/{}".format
KEY_COUNT="count"
PATTERN_SERVER=KEY_SERVER("*")


def server_add(address):
	p = redis.pipeline()
	p.set(KEY_SERVER(address), "")
	p.expire(KEY_SERVER(address), EXPIRE_SECONDS)
	p.execute()

def server_exists(address):
	return redis.get(KEY_SERVER(address)) != None

def server_count():
	count = redis.get(KEY_COUNT)
	if count is not None:
		return int(count.decode())
	count = sum(1 for _ in redis.scan_iter(PATTERN_SERVER))
	p = redis.pipeline()
	p.set(KEY_COUNT, count)
	p.expire(KEY_COUNT, 1)
	p.execute()
	return count

@app.route("/teeworlds/0.7/dynamic/register", methods=["POST"])
def register():
	json = request.get_json()
	if json is None:
		return abort(400)
	try:
		port = json["port"]
	except KeyError:
		return abort(400)

	return jsonify({
		"result": "fwcheck",
	})

@app.route("/teeworlds/0.7/dynamic/fwcheck", methods=["POST"])
def fwcheck():
	json = request.get_json()
	if json is None:
		return abort(400)
	try:
		port = json["port"]
	except KeyError:
		return abort(400)

	# TODO: Check if port is an integer.
	udp.sendto(b"\xff\xff\xff\x1f\xff\xff\xff\xff\xff\xfffw?2foobar\08303\0kekse\0", (request.remote_addr, port))

	#address = "{}:{}".format(request.remote_addr, port)
	#server_add(address)
	return jsonify({
		"result": "ok",
	})

@app.route("/teeworlds/0.7/dynamic/info")
def info():
	return jsonify({
		"count": server_count(),
	})

if __name__ == '__main__':
	app.debug = True
	app.run()
