

class Day:
    def __init__(self, date):
        self.date = date
        self.sunrise = None
        self.sunset = None

        _post_init()
    
    def _post_init(self):
        self.sunrise = calc_sunrise(self.date)
        self.sunset = calc_sunset(self.date)
    
    
    
