l4_5_protocol_both = Proto("Layer4.5", "Layer 4.5 Protocol for DNS front/end customization")

-- Header fields
proto_flag = ProtoField.uint32 ("l4_5_protocol_both.proto_flag", "protoFlag", base.HEX)
cust_data     = ProtoField.string("l4_5_protocol_both.cust_data"  , "Custom Data" , base.ASCII)
app_id     = ProtoField.string("l4_5_protocol_both.app_id"    , "Application ID" , base.ASCII)

l4_5_protocol_both.fields = { proto_flag, cust_data, app_id }


function l4_5_protocol_both.dissector(buffer, pinfo, tree)
    print("End dissector entry")
    found = -1
    length = buffer:len()
    if length <= 32 then return end

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
      udp_next(buffer(0, length):tvb(), pinfo, tree)
      return false
    end

    print("found")
    print(found)

    -- if found ==0, then front case, else end case

    if found == 0 then 
      pinfo.cols.protocol = l4_5_protocol_both.name
      local subtree = tree:add(l4_5_protocol_both, buffer(), "Layer4.5 Cust Protocol Data")
      -- Header
      subtree:add(proto_flag, buffer(0,4))
      subtree:add(app_id, buffer(4,16))

      local udp_next = Dissector.get("dns")
      -- print(table.concat(udp_next,", "))
      udp_next(buffer(20, length - 20):tvb(), pinfo, tree)
      return
    end
    
    data_length = length - found - 4

    print("data length")
    print(data_length)

    pinfo.cols.protocol = l4_5_protocol_both.name
    local subtree = tree:add(l4_5_protocol_both, buffer(), "Layer4.5 Cust Protocol End Data")
    -- Header (but need to find it at end of DNS query)

    subtree:add(proto_flag, buffer(found,4))
    subtree:add(cust_data, buffer(found+4,data_length))

    local udp_next = Dissector.get("dns")
    -- print(table.concat(udp_next,", "))
    udp_next(buffer(0, found):tvb(), pinfo, tree)
end

DissectorTable.get("udp.port"):add(53, l4_5_protocol_both)

