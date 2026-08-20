that is an exequtor

    - it should process sdk requests
    - should be like 
        callback -> raft -> sdk request -> raft -> response
        sdk requests can loop
    - so exequtor should choose a suitable raft host (it just knows its a host)
    - should be stateless just forvarding contexts