import json
import os
import tempfile
from redis import StrictRedis

redis = StrictRedis()

SERVER_PREFIX="server/"
INTERVAL=5

def dump(target, use_tempfile):
	servers = redis.scan_iter(match=SERVER_PREFIX+"*")
	servers = [s.decode()[len(SERVER_PREFIX):] for s in servers]
	if use_tempfile:
		with tempfile.NamedTemporaryFile('w', prefix=target+".", dir=".", delete=False) as f:
			json.dump(servers, f)
			os.rename(f.name, target)
	else:
		json.dump(servers, open(target, 'w'))

def main():
	import argparse
	import sched
	parser = argparse.ArgumentParser(description="Periodically dump the server list into a JSON file")
	parser.add_argument('--once', action='store_true', help="Only dump the server list once")
	parser.add_argument('target', metavar="TARGET", help="JSON file to create")
	args = parser.parse_args()

	def do():
		dump(args.target, True)
		s.enter(INTERVAL, 0, do)

	if not args.once:
		s = sched.scheduler()
		s.enter(0, 0, do)
		s.run()
	else:
		dump(args.target, False)

if __name__ == '__main__':
	main()
