# OpenRF Platform v0.2.3-alpha – Learn Engine

## Added
- Learn state machine: IDLE, WAITING_FOR_SIGNAL, PREVIEW_READY, ACCEPTED_RAM
- Controlled capture of the next valid RAW frame
- RAW preview before acceptance
- Accept and Discard REST actions
- Dedicated Learn page in WebUI

## API
- GET /api/radio/learn
- GET /api/radio/learn/raw
- POST /api/radio/learn/start
- POST /api/radio/learn/accept
- POST /api/radio/learn/discard

## Important
Accepted captures are stored in RAM only in v0.2.3. Persistent slot storage is intentionally not enabled yet.
