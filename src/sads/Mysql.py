import MySQLdb
import sys

from Utilities import Utilities

class SADS_Mysql:
    def __init__(self, user='root', passwd='Park0303', db='piwik', host='localhost'):
        self.user = user
        self.passwd = passwd
        self.db = db
        self.host = host               
        
    def connect(self):
        self.mysql = MySQLdb.connect(host=self.host, port=3306, user=self.user, passwd=self.passwd, db=self.db)
        self.cursor = self.mysql.cursor()

    def getVisitCount(self, location_ip=None, idvisitor=None,):
        query = "SELECT COUNT(*) FROM piwik_log_visit"
        condition = ""
        
        if location_ip != None:
            condition += "location_ip='%s'" % location_ip
        
        if condition != "":
            query = query + " WHERE " + condition
        
        print query
                
        self.cursor.execute(query)
        result = self.cursor.fetchall()
            
        return int(result[0][0])
        
    def test(self):
        ip = '127.0.0.1'
        
        Utilities.convertNumericIpToHex(ip)
        
        
        query = "SELECT location_ip from piwik_log_visit"
        self.cursor.execute(query)
        result = self.cursor.fetchall()
        
        print ip
        print result[0][0]
    
if __name__ == "__main__":
    mysql = SADS_Mysql()
    mysql.connect()
    mysql.getVisitCount()
    
    mysql.test()
    
    