import re

class Utilities:
    @staticmethod
    def convertNumericIpToHex(IPAddr):       
        data = re.split(r'\.', IPAddr)
        hexIp = '' 
                    
        for element in data:
            hexIp+=chr(int(element))                                        
        
        return hexIp