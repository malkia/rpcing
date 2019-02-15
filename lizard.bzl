def bask():
    for rule_name, rule in native.existing_rules().items():
        print( rule_name, rule )
        for k, v in rule.items():
            print( rule_name, k, type(v), v )