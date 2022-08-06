l4_5_protocol_both = Proto("L4.5_DNS", "Layer 4.5 Protocol for DNS front/end customization")

-- Header fields
proto_flag = ProtoField.uint32 ("l4_5_protocol_both.proto_flag", "protoFlag", base.HEX)
app_id     = ProtoField.string("l4_5_protocol_both.app_id"    , "App ID" , base.ASCII)

request     = ProtoField.string("l4_5_protocol_both.request"  , "Request" , base.ASCII)
xid     = ProtoField.uint16("l4_5_protocol_both.xid"  , "XID" , base.HEX)


l4_5_protocol_both.fields = { proto_flag, request, app_id, xid }


function l4_5_protocol_both.dissector(buffer, pinfo, tree)
    print("dissector entry")
    found = -1
    length = buffer:len()

    local udp_next = Dissector.get("dns")

    -- tag starts with XTAG, so try and find it
    -- flag = XTAG -> 0x58 54 41 47
    for i=0,length-4, 1 
    do 
      local potential_proto_flag = buffer(i,4):uint()
      if potential_proto_flag == 0x58544147 then
        found = i
        break
      end
    end

    if found == -1 then
      if length <= 32 then 
        -- dealing with compressed customization 
        print("found compress cust")
        print(found)
        pinfo.cols.protocol = l4_5_protocol_both.name
        local subtree = tree:add(l4_5_protocol_both, buffer(), "Layer4.5 Cust Protocol Data")
        -- Header
        subtree:add(xid, buffer(0,2))
        -- subtree:add(request, buffer(3,3))
        subtree:add(request, buffer(7,length-16))
        -- subtree:add(request, buffer(length-8, 3))
        return
      end
      if length > 32 then 
        local udp_next = Dissector.get("dns")
      
        udp_next(buffer(0, length):tvb(), pinfo, tree)
        return end
    end

    print("found tag cust")
    print(found)


    if found == 0 then 
      pinfo.cols.protocol = l4_5_protocol_both.name
      local subtree = tree:add(l4_5_protocol_both, buffer(), "Layer4.5 Cust Protocol Data")
      -- Header
      subtree:add(proto_flag, buffer(0,4))
      subtree:add(app_id, buffer(4,4))

      local udp_next = Dissector.get("dns")
      -- print(table.concat(udp_next,", "))
      udp_next(buffer(8, length - 8):tvb(), pinfo, tree)
      return
    end
    
end

DissectorTable.get("udp.port"):add(53, l4_5_protocol_both)