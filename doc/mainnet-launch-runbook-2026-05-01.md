# BlackRaven Mainnet Launch Runbook (2026-05-01)

This runbook is for launch-day operations only.  
Scope: release publication, node bootstrap, and first-block monitoring.

## T-24h: Final Lock

1. Freeze branch (`launch-kawpow-genesis`) for launch-critical fixes only.
2. Regenerate and verify release artifacts and checksums.
3. Validate seed host readiness and firewall rules.
4. Confirm explorer/indexer sync on final binaries.

## T-6h: Dry Run

1. Start at least 3 independent nodes from clean datadirs.
2. Verify peer discovery and inbound/outbound connectivity.
3. Validate RPC access controls on all operator nodes.
4. Confirm block template generation and mining client compatibility.

## T-60m: Go/No-Go

Required checks:
- Artifacts and checksums match approved values.
- Nodes are reachable from external hosts.
- Explorer and monitoring are green.
- Team is present in incident channel.

If any check fails, delay launch until fixed.

## T-0: Launch

1. Publish release assets and checksums.
2. Publish launch announcement and download links.
3. Start/confirm bootstrap nodes.
4. Confirm first peers and block flow.

## T+0 to T+120m: Live Monitoring

Track every 5-10 minutes:
- current height across bootstrap nodes
- peer counts
- mempool growth
- orphan/reorg events
- explorer parity with node RPC

Trigger incident response if:
- block production stalls beyond expected interval
- sustained chain split symptoms
- high orphan rate persists
- wallet crash/freeze reports become widespread

## Incident Response

1. Freeze any non-essential changes.
2. Record affected version, OS, command line, and logs.
3. Triage by severity:
   - P0: consensus/network safety
   - P1: wallet loss/corruption risk
   - P2: UX/performance issue
4. Communicate status every 30 minutes until resolved.

## Post-Launch (T+24h)

1. Publish first status report (network health metrics).
2. Archive final checksums and logs.
3. Start post-launch branch for upstream sync planning.
