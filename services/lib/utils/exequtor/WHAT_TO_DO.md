that is an executor

    - it should process sdk requests
    - should be like 
        callback -> raft -> sdk request -> raft -> response
        sdk requests can loop
    - so executor should choose a suitable raft host (it just knows its a host)
    - should be stateless just forvarding contexts